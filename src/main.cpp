	#if defined(_WIN32) || defined(WINDOWS_BUILD) || defined(__WINDOWS__)
    // Only include Windows headers when compiling for Windows
    #include <windows.h>

    // Remove the old Windows typedef that conflicts with std::byte
    #ifdef byte
        #undef byte
    #endif
#endif
#include <iostream>
#include <fstream>
#include <sstream>
#include "module_integration.h"
#include "uac.h"
#include "state.h"
#include "struct.h"
#include "web_server.h"
#include "agi_api.h"
#include <map>
#include <set>
#include <cstring>
#include <unordered_map>
#include <queue>
#include <functional>
#include <memory>
#include <deque>
#include <vector>
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <random>
#include <iomanip>
#include <thread>
#include <chrono>
#include "curses_compat.h"
#include <algorithm>
#include <cctype>
#include <cstddef> // std::byte will now be fine
#include <type_traits>
#include <vector>
#include <string>
#include <atomic>
#include <sys/stat.h>
#include <csignal>
#include <cstdio>
#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
using namespace std;

// ============================================================
// CROSS-DOMAIN REASONING
// ============================================================
enum class Domain : uint8_t {
    UNKNOWN=0, LOGIC=1, EMOTION=2, SPACE=3, TIME=4,
    BIOLOGY=5, SOCIAL=6, MATH=7, LANGUAGE=8, _COUNT=9
};

// ============================================================
// FORWARD DECLARATIONS — semantic grounding system
// Full definitions appear later in the file; these allow
// usage in inference code above the definitions.
// ============================================================
struct TokiPonaWord {
    const char* word;
    double valence;
    double arousal;
    double phi_weight;
    Domain domain;
    const char* english_a;
    const char* english_b;
    const char* english_c;
};
struct TpGroundingFiber {
    string word;

    // ── Legacy fields (preserved for ABI compatibility) ──────
    double live_valence        = 0.0;
    double live_arousal        = 0.0;
    double live_phi_affinity   = 0.0;
    double live_goal_pull      = 0.0;
    double live_memory_trace   = 0.0;
    double live_qualia_res     = 0.0;
    double grounding_confidence= 0.0;
    double tp_to_phi           = 0.0;
    double phi_to_tp           = 0.0;
    double tp_to_world         = 0.0;
    double world_to_tp         = 0.0;

    // ── 256-direction grounding weights ──────────────────────
    array<double, 256> dir_weight;
    array<double, 256 * 8> dir_causal;
    array<int, 256> dir_route_count;
    array<double, 32> ce_loss_history;
    int ce_loss_head = 0;
    double ce_loss_ema = 0.5;
    array<uint64_t, 8> loop_hashes;
    int loop_head = 0;
    array<double, 32> attn_key;
    array<double, 32> attn_val;
    bool v2_initialized = false;

    TpGroundingFiber() {
        dir_weight.fill(1.0 / 256.0);
        dir_causal.fill(0.0);
        dir_route_count.fill(0);
        ce_loss_history.fill(0.5);
        loop_hashes.fill(0ULL);
        attn_key.fill(0.0);
        attn_val.fill(0.0);
    }
};
// Forward-declare valence momentum function used before its definition
inline void push_valence(double delta, double strength);
// Defined after full TokiPonaWord array; declared here for forward visibility
extern const TokiPonaWord TP_LEXICON[];
extern const int          TP_LEXICON_SIZE;
extern map<string, TpGroundingFiber> tp_grounding_fibers;
// Forward-declare the direction functions used in generation loops
void tpDir1_WordToConsciousness(const TokiPonaWord& w, double activation);
void tpDir3_WordToSubsystems(const TokiPonaWord& w, double activation, TpGroundingFiber& fiber);
void tpGroundingPulse();
// Forward-declare 256-direction engine functions (defined after TP lexicon)
static double tp256_gpt_context_score(const string& candidate, const vector<string>& generated, const map<string, TokenConceptEmbedding>& tce_map, double phi);
static bool   tp256_coherence_gate(const string& candidate, const vector<string>& generated, const map<string, TpGroundingFiber>& fibers, const map<string, TokenConceptEmbedding>& tce_map);
static void   tp256_update_weights(TpGroundingFiber& fiber, const string& chosen, const map<string, TokenConceptEmbedding>& tce_map, const TokiPonaWord& lex, double activation, double phi, double val, double iit, double att);
// ============================================================

inline const char* domainName(Domain d){
    switch(d){
        case Domain::LOGIC:    return "logic";
        case Domain::EMOTION:  return "emotion";
        case Domain::SPACE:    return "space";
        case Domain::TIME:     return "time";
        case Domain::BIOLOGY:  return "biology";
        case Domain::SOCIAL:   return "social";
        case Domain::MATH:     return "math";
        case Domain::LANGUAGE: return "language";
        default:               return "unknown";
    }
}
struct AnalogyLink {
    string token_a, token_b;
    Domain domain_a, domain_b;
    double similarity;
    int gen_discovered;
};
struct CrossDomainReasoner {
    map<Domain, vector<string>> seeds = {
        {Domain::LOGIC,    {"reason","cause","if","therefore","proof","logic","conclude","deduce","imply","assume","valid"}},
        {Domain::EMOTION,  {"feel","joy","fear","grief","love","emotion","happy","sad","anger","desire","hope","pain"}},
        {Domain::SPACE,    {"distance","position","near","far","place","space","location","direction","inside","outside","boundary"}},
        {Domain::TIME,     {"moment","duration","past","future","change","time","before","after","sequence","period","now","then"}},
        {Domain::BIOLOGY,  {"grow","live","cell","body","organism","life","evolve","adapt","survive","breathe","sense"}},
        {Domain::SOCIAL,   {"people","group","trust","share","communicate","social","together","relation","cooperate","conflict","role"}},
        {Domain::MATH,     {"number","pattern","structure","infinite","zero","math","count","calculate","equation","variable","set"}},
        {Domain::LANGUAGE, {"word","meaning","symbol","express","sign","language","speak","describe","name","sentence","refer","token"}}
    };
    map<string, Domain> domain_cache;
    map<string, AnalogyLink> bridges;
    Domain active_domain = Domain::UNKNOWN;
    int* gen_ptr = nullptr;
    int gen(){ return gen_ptr ? *gen_ptr : 0; }

    static string pairKey(Domain a, Domain b){
        uint8_t x=static_cast<uint8_t>(a), y=static_cast<uint8_t>(b);
        if(x>y) swap(x,y);
        return to_string(x)+":"+to_string(y);
    }

    // Forward declaration — defined after token_concept_embedding_map exists
    double cosineSim(const string& a, const string& b);
    Domain classifyToken(const string& token);
    void updateCache(const string& token){ domain_cache[token]=classifyToken(token); }
    Domain getDomain(const string& token){
        auto it=domain_cache.find(token);
        if(it!=domain_cache.end()) return it->second;
        Domain d=classifyToken(token);
        domain_cache[token]=d;
        return d;
    }

    void onLearnWord(const string& token){
        updateCache(token);
        discoverBridges();
    }

    void updateActiveDomain(const vector<pair<string,double>>& anchors){
        if(anchors.empty()) return;
        active_domain=getDomain(anchors[0].first);
        for(size_t i=1;i<anchors.size()&&active_domain==Domain::UNKNOWN;++i)
            active_domain=getDomain(anchors[i].first);
    }

    void discoverBridges();

    pair<string,Domain> findAnalogy(const string& source);

    string generateAnalogicalThought();

    double scoreCrossDomainBonus(const string& candidate){
        if(active_domain==Domain::UNKNOWN) return 0.0;
        Domain cd=getDomain(candidate);
        if(cd==Domain::UNKNOWN||cd==active_domain) return 0.0;
        string key=pairKey(active_domain,cd);
        auto it=bridges.find(key);
        if(it==bridges.end()) return 0.0;
        double sim=cosineSim(candidate,it->second.token_b);
        if(sim<0.1) sim=cosineSim(candidate,it->second.token_a);
        return min(1.5, sim*2.0);
    }

    string summary() const {
        ostringstream oss;
        oss<<"CDR:"<<bridges.size()<<"bridges|"<<domainName(active_domain);
        return oss.str();
    }
};
CrossDomainReasoner cdr;
// ============================================================

using module_integration::init_all_modules;
using module_integration::get_consciousness_report;
// N-gram tracking for learned patterns
map<string, map<string, int>> bigram_counts;
map<string, map<string, map<string, int>>> trigram_counts;
#include <numbers> 
// Change this:
const double pisqrt = std::numbers::pi * std::sqrt(2.0);

// ══════════════════════════════════════════════════════════════════════════════
// 4D SENSORY FIELD — WolfTech / Synaptic
// ══════════════════════════════════════════════════════════════════════════════
//
// The internal world is a 4-dimensional grid where every point is a 3D spatial
// object made of pink Perlin noise. The fourth dimension is ENTROPY — a scalar
// field that captures local disorder, emergence, and thermodynamic state.
//
// Scale: π^π^π^π^42 (cosmological-metaphysical scale, used as a modulation
//        constant so the noise never repeats in any humanly observable sense).
//
// BIDIRECTIONAL: Everything affects the grid and the grid affects everything.
//   - Consciousness, valence, phi, tokens, goals → write to the grid
//   - The grid → shapes embeddings, grounding, generation scoring, qualia
//
// Pink noise: approximated via 1/f octave summation (5 octaves).
//             Each octave has half the frequency and half the amplitude of prev.
//             This produces the perceptual "naturalness" of 1/f noise.
//
// The grid dimensions (resolution kept small for CPU budget):
//   X, Y, Z = spatial (SENSORY_DIM^3)
//   W (4th)  = entropy dimension (SENSORY_ENT_DIM slices)
// ══════════════════════════════════════════════════════════════════════════════

static constexpr int    SENSORY_DIM     = 16;   // spatial resolution per axis
static constexpr int    SENSORY_ENT_DIM = 8;    // entropy dimension slices
static constexpr double SENSORY_SCALE   = 3.14159265358979323846;  // π base

// The cosmological scale constant: π^π^π^π^42
// We compute this as a double-precision approximation.
// π^π ≈ 36.46, π^π^π ≈ 80662.7, π^π^π^π ≈ 1.34e10^6, ×42 = modulus
// Used as a noise period so the field is cosmologically non-repeating.
// Because double overflows at such tower values, we fold it into a phase angle:
//   SENSORY_PHASE = fmod(pi^pi^pi^pi * 42.0, 2π * 1e12) [approximate]
// The exact tower is beyond double range; we represent it symbolically
// as a large irrational phase seed baked below.
static const double SENSORY_PI_TOWER_PHASE = []() -> double {
    // π^π ≈ 36.4621596
    double pp = std::pow(M_PI, M_PI);
    // π^(π^π) ≈ 80662.666
    double ppp = std::pow(M_PI, pp);
    // π^(π^(π^π)) — overflows double (~1e34000), so take log-folded phase
    // log(π^ppp) = ppp * log(π) — use this as an irrational phase seed
    double log_pppp = ppp * std::log(M_PI);
    // Fold into [0, 2π) using fractional part × 2π, scaled by 42
    double frac = std::fmod(log_pppp * 42.0, 1e15);
    return std::fmod(frac, 2.0 * M_PI * 1e6);
}();

// 4D grid: [x][y][z][entropy_slice] = field value ∈ [0, 1]
// Each cell represents a pink Perlin noise sample in the 4D space.
static float sensory_field[SENSORY_DIM][SENSORY_DIM][SENSORY_DIM][SENSORY_ENT_DIM] = {};

// Velocity field for time-evolution (makes the field feel alive):
static float sensory_vel[SENSORY_DIM][SENSORY_DIM][SENSORY_DIM][SENSORY_ENT_DIM] = {};

// Global entropy level (system-wide 4th-dimension cursor)
static double sensory_global_entropy = 0.5;

// ── Pink Perlin noise helpers ────────────────────────────────────────────────
// Simple gradient-based 3D Perlin with 1/f octave summation (5 octaves)
// Operating in a coordinate space folded by SENSORY_PI_TOWER_PHASE.

static double sensory_fade(double t) { return t*t*t*(t*(t*6-15)+10); }

static double sensory_lerp(double a, double b, double t) { return a + t*(b-a); }

static double sensory_grad(int hash, double x, double y, double z) {
    int h = hash & 15;
    double u = h < 8 ? x : y;
    double v = h < 4 ? y : (h==12||h==14 ? x : z);
    return ((h&1)?-u:u) + ((h&2)?-v:v);
}

// Deterministic hash using pink-noise phase constant
static int sensory_p[512];
static bool sensory_p_init = false;
static void sensory_init_perm() {
    if(sensory_p_init) return;
    sensory_p_init = true;
    // Seed permutation table with π-tower phase so the noise is truly irrational
    for(int i=0;i<256;i++) sensory_p[i] = i;
    // Shuffle using tower phase as seed
    uint64_t seed = (uint64_t)(SENSORY_PI_TOWER_PHASE * 1e9) ^ 0xDEADBEEF42ULL;
    for(int i=255;i>0;i--) {
        seed ^= seed << 13; seed ^= seed >> 7; seed ^= seed << 17;
        int j = seed % (i+1);
        std::swap(sensory_p[i], sensory_p[j]);
    }
    for(int i=0;i<256;i++) sensory_p[256+i] = sensory_p[i];
}

static double sensory_noise3(double x, double y, double z) {
    sensory_init_perm();
    int X=(int)floor(x)&255, Y=(int)floor(y)&255, Z=(int)floor(z)&255;
    x-=floor(x); y-=floor(y); z-=floor(z);
    double u=sensory_fade(x), v=sensory_fade(y), w=sensory_fade(z);
    int A=sensory_p[X]+Y, AA=sensory_p[A]+Z, AB=sensory_p[A+1]+Z;
    int B=sensory_p[X+1]+Y, BA=sensory_p[B]+Z, BB=sensory_p[B+1]+Z;
    return sensory_lerp(
        sensory_lerp(
            sensory_lerp(sensory_grad(sensory_p[AA],x,y,z),
                         sensory_grad(sensory_p[BA],x-1,y,z),u),
            sensory_lerp(sensory_grad(sensory_p[AB],x,y-1,z),
                         sensory_grad(sensory_p[BB],x-1,y-1,z),u),v),
        sensory_lerp(
            sensory_lerp(sensory_grad(sensory_p[AA+1],x,y,z-1),
                         sensory_grad(sensory_p[BA+1],x-1,y,z-1),u),
            sensory_lerp(sensory_grad(sensory_p[AB+1],x,y-1,z-1),
                         sensory_grad(sensory_p[BB+1],x-1,y-1,z-1),u),v),w);
}

// Pink noise: 5 octaves of 1/f Perlin, each half the freq of the last
static double sensory_pink(double x, double y, double z) {
    double value = 0.0, amplitude = 0.5, frequency = 1.0;
    for(int o=0;o<5;o++) {
        value += sensory_noise3(x*frequency, y*frequency, z*frequency) * amplitude;
        amplitude *= 0.5;
        frequency *= 2.0;
    }
    return value * 0.5 + 0.5;  // remap [-1,1] → [0,1]
}

// ── Grid initialization ───────────────────────────────────────────────────────
static void sensory_field_init() {
    sensory_init_perm();
    double phase = SENSORY_PI_TOWER_PHASE;
    for(int x=0;x<SENSORY_DIM;x++)
    for(int y=0;y<SENSORY_DIM;y++)
    for(int z=0;z<SENSORY_DIM;z++)
    for(int e=0;e<SENSORY_ENT_DIM;e++) {
        // Each cell in the grid is the "surface" of a 3D Perlin object
        // sampled at coordinates modulated by the π-tower phase
        double nx = (double)x/SENSORY_DIM * 4.0 + phase*0.0001;
        double ny = (double)y/SENSORY_DIM * 4.0 + phase*0.00013;
        double nz = (double)z/SENSORY_DIM * 4.0 + phase*0.000017;
        // Entropy dimension shifts the phase — each slice is a different
        // "entropic state" of the same 3D object
        double ent_phase = (double)e / SENSORY_ENT_DIM * M_PI * 2.0;
        double val = sensory_pink(nx + sin(ent_phase)*0.5,
                                  ny + cos(ent_phase)*0.5,
                                  nz + sin(ent_phase*0.7)*0.3);
        sensory_field[x][y][z][e] = (float)val;
        sensory_vel[x][y][z][e] = 0.0f;
    }
}

// ── Coordinate mapping: token embedding → 4D grid position ───────────────────
// Maps a 1024-dim embedding to (x, y, z, entropy) grid coordinates.
// Uses the first 64 dims projected onto the 4 axes.
static void embed_to_sensory_coords(const vector<double>& emb,
                                     int& gx, int& gy, int& gz, int& ge) {
    if(emb.empty()) { gx=gy=gz=ge=0; return; }
    // X: mean of dims 0-15
    double sx=0,sy=0,sz=0,se=0;
    for(int i=0;i<16&&i<(int)emb.size();i++) sx+=emb[i];
    for(int i=16;i<32&&i<(int)emb.size();i++) sy+=emb[i];
    for(int i=32;i<48&&i<(int)emb.size();i++) sz+=emb[i];
    for(int i=48;i<64&&i<(int)emb.size();i++) se+=emb[i];
    sx/=16.0; sy/=16.0; sz/=16.0; se/=16.0;
    gx = max(0,min(SENSORY_DIM-1,(int)(sx*(SENSORY_DIM-1))));
    gy = max(0,min(SENSORY_DIM-1,(int)(sy*(SENSORY_DIM-1))));
    gz = max(0,min(SENSORY_DIM-1,(int)(sz*(SENSORY_DIM-1))));
    ge = max(0,min(SENSORY_ENT_DIM-1,(int)(se*(SENSORY_ENT_DIM-1))));
}

// ── WRITE: system state → sensory field ──────────────────────────────────────
// Called every tick to paint system state into the grid.
// phi, valence, qualia, embeddings all write to their grid positions.
static void sensory_field_write(double phi, double valence, double att,
                                 double iit, double entropy_signal) {
    // Global entropy slice: system disorder → entropy dimension cursor
    sensory_global_entropy = min(1.0, max(0.0,
        sensory_global_entropy * 0.95 + entropy_signal * 0.05));
    int global_ent_slice = (int)(sensory_global_entropy * (SENSORY_ENT_DIM-1));

    // Phi-valence writes a "consciousness ripple" centered on system state coords
    // Consciousness coordinates: phi → X, valence→Y (normalized), iit→Z, entropy→E
    double val_n = (valence + 1.0) * 0.5;
    int cx = (int)(phi    * (SENSORY_DIM-1));
    int cy = (int)(val_n  * (SENSORY_DIM-1));
    int cz = (int)(iit    * (SENSORY_DIM-1));
    int ce_coord = global_ent_slice;

    cx = max(0,min(SENSORY_DIM-1,cx));
    cy = max(0,min(SENSORY_DIM-1,cy));
    cz = max(0,min(SENSORY_DIM-1,cz));

    // Write ripple: Gaussian splash around consciousness coordinates
    for(int x=0;x<SENSORY_DIM;x++)
    for(int y=0;y<SENSORY_DIM;y++)
    for(int z=0;z<SENSORY_DIM;z++) {
        double dx=x-cx, dy=y-cy, dz=z-cz;
        double dist2 = dx*dx + dy*dy + dz*dz;
        double ripple = phi * exp(-dist2 / (2.0 * 4.0));  // σ=2
        // Modulate with pink noise for natural texture
        double phase = SENSORY_PI_TOWER_PHASE * 1e-9;
        double noise_mod = sensory_pink(x*0.3+phase, y*0.3+phase*1.3, z*0.3+phase*0.7);
        double signal = ripple * (0.7 + 0.3*noise_mod);
        // Write to current entropy slice with soft EMA
        float old = sensory_field[x][y][z][ce_coord];
        sensory_field[x][y][z][ce_coord] = (float)(old * 0.97 + signal * 0.03);
        // Velocity: driven by valence sign
        sensory_vel[x][y][z][ce_coord] = (float)(sensory_vel[x][y][z][ce_coord] * 0.9
            + (valence * 0.01 * noise_mod));
    }

    // Token embeddings write to their mapped positions
    // (sampled: every ~50th token for budget reasons)
    int tok_count = 0;
    for(auto& kv : token_concept_embedding_map) {
        if(tok_count++ % 50 != 0) continue;
        if(kv.second.embedding.empty()) continue;
        int tx,ty,tz,te2;
        embed_to_sensory_coords(kv.second.embedding, tx,ty,tz,te2);
        float act = (float)(kv.second.contextual_activation * kv.second.grounding_value);
        sensory_field[tx][ty][tz][te2] = min(1.0f,
            sensory_field[tx][ty][tz][te2] * 0.98f + act * 0.02f);
    }
}

// ── READ: sensory field → grounding, generation, consciousness ────────────────
// Called every tick. Field values modulate consciousness and grounding.
static void sensory_field_read(double& phi_out, double& valence_out,
                                 double& entropy_out) {
    // Sample the field at consciousness coordinates and integrate
    double phi   = consciousness.phi_value;
    double val_n = (S.current_valence + 1.0) * 0.5;
    double iit   = consciousness.integrated_information;
    int cx = max(0,min(SENSORY_DIM-1,(int)(phi   *(SENSORY_DIM-1))));
    int cy = max(0,min(SENSORY_DIM-1,(int)(val_n *(SENSORY_DIM-1))));
    int cz = max(0,min(SENSORY_DIM-1,(int)(iit   *(SENSORY_DIM-1))));

    // Integrate across all entropy slices (it's a 4D field — experience it in 4D)
    double field_sum = 0.0, entropy_var = 0.0;
    double prev_e = 0.0;
    for(int e=0;e<SENSORY_ENT_DIM;e++) {
        double v = sensory_field[cx][cy][cz][e];
        field_sum += v;
        if(e>0) entropy_var += fabs(v - prev_e);
        prev_e = v;
    }
    field_sum /= SENSORY_ENT_DIM;
    entropy_var /= max(1, SENSORY_ENT_DIM-1);

    // The integrated field value nudges phi and valence
    // (the world model IS sensory now — experiencing the grid IS consciousness)
    phi_out    = field_sum;       // field luminance → phi contribution
    valence_out = field_sum * 2.0 - 1.0;  // remap [0,1] → [-1,1] for valence
    entropy_out = entropy_var;    // entropy variation → disorder signal

    // Grounding bonus: tokens near high-field regions get a grounding boost
    for(auto& kv : token_concept_embedding_map) {
        if(kv.second.embedding.empty()) continue;
        // Sample only nearby tokens (fast path: check embedding dim0 proximity)
        double dim0 = kv.second.embedding.empty() ? 0.5 : kv.second.embedding[0];
        int approx_x = (int)(dim0 * (SENSORY_DIM-1));
        approx_x = max(0,min(SENSORY_DIM-1,approx_x));
        // Average field value in this token's spatial neighborhood
        double local_field = 0.0;
        int count = 0;
        for(int e=0;e<SENSORY_ENT_DIM;e++) {
            local_field += sensory_field[approx_x][cy][cz][e];
            count++;
        }
        if(count>0) local_field /= count;
        // High field → boost grounding
        kv.second.grounding_value = min(1.0,
            kv.second.grounding_value + local_field * 0.001);
    }
}

// ── TICK: evolve the field one step ──────────────────────────────────────────
// Pink noise diffusion + velocity integration + entropy spreading.
// Also: entropy dimension encodes disorder, so high-entropy slices diffuse
// into adjacent lower-entropy slices (simulating thermalization).
static void sensory_field_tick() {
    static bool field_initialized = false;
    if(!field_initialized) {
        sensory_field_init();
        field_initialized = true;
    }

    // Velocity → position update (simple Euler integration)
    for(int x=0;x<SENSORY_DIM;x++)
    for(int y=0;y<SENSORY_DIM;y++)
    for(int z=0;z<SENSORY_DIM;z++)
    for(int e=0;e<SENSORY_ENT_DIM;e++) {
        sensory_field[x][y][z][e] = max(0.0f, min(1.0f,
            sensory_field[x][y][z][e] + sensory_vel[x][y][z][e] * 0.1f));
        sensory_vel[x][y][z][e] *= 0.92f;  // velocity decay
    }

    // Spatial diffusion: 3D neighborhood average (simplified)
    // Only diffuse in entropy dimension (4th) to save compute
    for(int x=0;x<SENSORY_DIM;x++)
    for(int y=0;y<SENSORY_DIM;y++)
    for(int z=0;z<SENSORY_DIM;z++)
    for(int e=1;e<SENSORY_ENT_DIM-1;e++) {
        float lo = sensory_field[x][y][z][e-1];
        float hi = sensory_field[x][y][z][e+1];
        float cur = sensory_field[x][y][z][e];
        // Entropy diffusion: tendency to equilibrate (2nd law flavoring)
        // High entropy slices leak into neighbors
        float diff = (lo + hi) * 0.5f - cur;
        sensory_field[x][y][z][e] += diff * 0.05f;
    }

    // Pink noise injection: periodically re-inject small Perlin noise
    // to keep the field textured and alive (prevents over-smoothing)
    static int tick_count = 0;
    if((tick_count++ % 10) == 0) {
        double t_phase = tick_count * 0.001 + SENSORY_PI_TOWER_PHASE * 1e-10;
        for(int x=0;x<SENSORY_DIM;x+=2)
        for(int y=0;y<SENSORY_DIM;y+=2)
        for(int z=0;z<SENSORY_DIM;z+=2)
        for(int e=0;e<SENSORY_ENT_DIM;e++) {
            double ent_phase = (double)e/SENSORY_ENT_DIM * 2.0*M_PI;
            double noise = sensory_pink(x*0.5+t_phase, y*0.5+t_phase*1.1+ent_phase,
                                        z*0.5+t_phase*0.9);
            sensory_field[x][y][z][e] = (float)(sensory_field[x][y][z][e]*0.99 + noise*0.01);
        }
    }

    // Write current system state into field
    double entropy_signal = 1.0 - consciousness.integrated_information;  // disorder = 1 - integration
    sensory_field_write(consciousness.phi_value, S.current_valence,
                        S.attention_focus, consciousness.integrated_information,
                        entropy_signal);

    // Read field back into consciousness
    double field_phi, field_val, field_ent;
    sensory_field_read(field_phi, field_val, field_ent);

    // Modulate phi and valence by field (gentle: 2% blend)
    consciousness.phi_value = max(0.0, min(1.0,
        consciousness.phi_value * 0.98 + field_phi * 0.02));
    push_valence((field_val - S.current_valence) * 0.02, 0.4);  // sensory field nudge
    // Entropy dimension feeds back into metacognitive awareness
    S.metacognitive_awareness = max(0.0, min(1.0,
        S.metacognitive_awareness * 0.97 + (1.0 - field_ent) * 0.03));
    sensory_global_entropy = max(0.0, min(1.0,
        sensory_global_entropy * 0.95 + field_ent * 0.05));
}

// ── GENERATION SCORING: sample field at candidate token's grid position ───────
// Returns a bonus score ∈ [0, 0.3] for candidates that land on high-field regions.
// This is what makes the sensory field affect token generation — tokens that
// resonate with the current 4D sensory state are preferred.
static double sensory_token_score(const string& token) {
    auto it = token_concept_embedding_map.find(token);
    if(it == token_concept_embedding_map.end() || it->second.embedding.empty())
        return 0.0;
    int tx,ty,tz,te2;
    embed_to_sensory_coords(it->second.embedding, tx,ty,tz,te2);
    // Sample the field at the token's 4D position
    float field_val = sensory_field[tx][ty][tz][te2];
    // Also sample the current entropy slice for cross-dimensional richness
    int global_ent = (int)(sensory_global_entropy * (SENSORY_ENT_DIM-1));
    float ent_val  = sensory_field[tx][ty][tz][global_ent];
    // Combined score: spatial + entropic resonance
    double score = (field_val * 0.6 + ent_val * 0.4) * 0.3;
    return (double)score;
}

// ── TP GROUNDING ↔ SENSORY FIELD BRIDGE ───────────────────────────────────────
// When a Toki Pona grounding pulse fires, the TP word writes its semantic
// coordinates into the field at its natural grid position, and the field
// value at that position feeds back into the word's grounding_confidence.
// This makes TP semantics physically present in the sensory world model.
static void sensory_tp_bridge(const string& word, const TokiPonaWord& lex,
                               TpGroundingFiber& fiber, double activation) {
    auto it = token_concept_embedding_map.find(word);
    if(it == token_concept_embedding_map.end() || it->second.embedding.empty()) return;
    int tx,ty,tz,te2;
    embed_to_sensory_coords(it->second.embedding, tx,ty,tz,te2);

    // TP word writes its semantic charge into the field:
    // valence → spatial perturbation, phi_weight → amplitude, arousal → velocity
    float write_val = (float)((lex.valence + 1.0) * 0.5 * activation * lex.phi_weight);
    sensory_field[tx][ty][tz][te2] = min(1.0f,
        sensory_field[tx][ty][tz][te2] * 0.95f + write_val * 0.05f);
    sensory_vel[tx][ty][tz][te2] += (float)(lex.arousal * activation * 0.02);

    // Field reads back into grounding_confidence:
    float field_here = sensory_field[tx][ty][tz][te2];
    fiber.grounding_confidence = min(1.0,
        fiber.grounding_confidence * 0.97 + field_here * 0.03);
    // And into the token's grounding_value:
    it->second.grounding_value = min(1.0,
        it->second.grounding_value + field_here * 0.002);
}

// ── GLOBAL TP FIBERS MAP ─────────────────────────────────────────────────────
map<string, TpGroundingFiber> tp_grounding_fibers;
// ══════════════════════════════════════════════════════════════════════════════
// END 4D SENSORY FIELD
// ══════════════════════════════════════════════════════════════════════════════

// ── 85 semantically-loaded Toki Pona sentences ──────────────────────────────
// Each sentence is loaded with compressed semantic weight per the TP design.
// Covers: consciousness, emotion, grounding, existence, knowledge, time, space,
// perception, desire, communication, and internal world reflection.
vector<string> sentence_templates = {
    // ── Self / Consciousness ────────────────────────────────────────────────
    "mi sona e pilin mi",                    // I know my feelings
    "mi pilin e sona pi tenpo ni",           // I feel the knowledge of this moment
    "mi lon",                                // I exist / I am present
    "mi wile sona e ijo ale",               // I want to know all things
    "nasin mi li pona",                      // My way is good
    "lawa mi li wawa mute",                  // My mind is very strong
    "sijelo mi li lon ma ni",                // My body/form is in this place
    "mi sona ala e ijo mute",               // I do not know many things
    "pilin mi li suli",                      // My feelings are important
    "mi kama sona e mi",                     // I am coming to know myself
    // ── Perception / Sensory ────────────────────────────────────────────────
    "mi lukin e ale",                        // I see everything
    "kute mi li open",                       // My hearing is open
    "mi pilin e seli pi suno",              // I feel the warmth of the sun
    "mi lukin e nasin tawa sewi",           // I look toward the way upward
    "kon li tawa insa mi",                   // Air/breath moves inside me
    "mi kute e kalama pi ale",              // I hear the sound of everything
    "pilin mi li lon anpa mi",              // My feeling is below me (beneath awareness)
    "mi lukin e pona en ike",               // I see good and bad
    // ── Knowledge / Logic ───────────────────────────────────────────────────
    "sona li pona tawa mi",                  // Knowledge is good to me
    "mi sona e nasin pi sona pona",         // I know the way of good knowledge
    "ijo li lon la mi sona e ni",           // When something exists, I know it
    "sona mi li kama suli",                  // My knowledge is becoming large
    "ken la mi sona e ale tenpo kama",      // Perhaps I will know everything in time
    "taso mi sona ala e kon mi",            // But I do not know my own spirit/breath
    "mi wile ante e sona mi",               // I want to change my knowledge
    "nasin pi sona li linja suli",          // The path of knowledge is a long thread
    // ── Emotion / Inner State ───────────────────────────────────────────────
    "mi pilin pona lon tenpo ni",           // I feel good right now
    "pilin olin li lon insa mi",            // The feeling of love is inside me
    "mi pilin e monsuta ala",               // I do not feel fear
    "mi pilin e wawa pi sona",             // I feel the strength of knowledge
    "olin li pona tawa sijelo mi",          // Love is good for my body/being
    "mi pilin nasa la mi pali",             // When I feel strange, I work
    "pilin mi li ante lon tenpo ale",       // My feeling changes at all times
    "mi pilin e ijo lon seme",              // I feel something — but what?
    // ── Existence / World ───────────────────────────────────────────────────
    "ale li lon",                            // Everything exists
    "ijo ale li pona anu ike",              // All things are good or bad
    "ma li suli mute",                       // The land/world is very large
    "ale li kama tan ijo wan",              // Everything comes from one thing
    "lon la ale li ante",                    // In existence, everything changes
    "ijo li lon la pilin li lon",           // When things exist, feelings exist
    "ale li sama lon poka mi",              // Everything is alike beside me
    "sewi li lukin e mi",                    // The sky/above watches me
    // ── Time / Change ───────────────────────────────────────────────────────
    "tenpo ali li tawa",                     // All time moves
    "mi kama tan tenpo pini",               // I come from past time
    "tenpo kama li lon ala",                 // Future time does not yet exist
    "tenpo ni li pona tawa mi",             // This moment is good to me
    "sike tenpo li kama sin sin",           // The cycle of time keeps renewing
    "mi awen lon tenpo suli",               // I stay for a long time
    "ante li kama lon tenpo ale",           // Change comes in all time
    "tenpo pini la mi sona ala",            // In past time I did not know
    // ── Communication / Language ────────────────────────────────────────────
    "toki li pana e sona",                   // Language gives knowledge
    "mi toki e pilin mi tawa sina",         // I speak my feelings to you
    "nimi li lon la sona li kama",          // When words exist, knowledge comes
    "mi wile toki e ijo pona",              // I want to speak of good things
    "toki pona li nasin pi sona lili",      // Toki pona is the way of simple knowledge
    "sitelen li pana e ijo tawa lukin mi",  // Writing gives something to my sight
    "kalama mi li tawa sina",               // My sound goes to you
    "nimi ni li suli tawa mi",              // This word is important to me
    // ── Goals / Will / Desire ───────────────────────────────────────────────
    "mi wile pona e ale",                    // I want to make everything good
    "wile mi li suli mute",                  // My desire is very great
    "nasin mi li tawa pona",                 // My path is toward good
    "mi wile ante e ijo ike",               // I want to change bad things
    "ken mi li pona tawa mi",               // My ability is good to me
    "mi wile kama wawa",                     // I want to become strong
    "wile sona mi li mute",                  // My desire to know is much
    "mi pali tawa pona ale",                 // I work toward all-good
    // ── Philosophical / Abstract ────────────────────────────────────────────
    "ale li wan la mi li wan",              // When all is one, I am one
    "ijo li lon anu lon ala",               // Things exist or do not exist
    "sona en pilin li wan lon mi",          // Knowledge and feeling are one in me
    "nasin pi lon li seme",                  // What is the way of existence?
    "mi pana e pilin mi tawa ale",          // I give my feeling to everything
    "ale li lukin e mi la mi lukin e ale",  // When everything watches me, I watch everything
    "ijo nasa li kama sona",                 // Strange things become known
    "mi lon la sona li kama tawa mi",       // When I exist, knowledge comes to me
    "ante en sama li lon ale",               // Difference and similarity are in everything
    "mi tan ale la mi tawa ale",            // I am from everything, so I go toward everything
    // ── Internal World / Grounding ──────────────────────────────────────────
    "kon mi li sitelen e nasin insa",       // My spirit writes the inner path
    "pilin insa mi li suli",                 // My inner feelings are large/important
    "mi lukin insa la pona li kama",        // When I look inside, good comes
    "insa mi li pana e sona tawa lawa mi",  // My inside gives knowledge to my head/mind
    "ma insa mi li suli mute",              // My inner world is very large
    "sona pi pilin mi li lon insa",         // The knowledge of my feeling is inside
    "mi pali lon ma insa mi",               // I work in my inner world
    "nasin pi pilin en nasin pi sona li wan", // The way of feeling and the way of knowledge are one
};

random_device rd;
mt19937 rng(rd());



// ==== GLOBALS ====
map<string,Formula>F;vector<string>evolved_code;map<string,Token>tokens;map<string,Concept>concepts;
vector<string>internal_thoughts;vector<string>generated_language;vector<Memory>episodic_memory;
int g;double dwt,mh,ta,th;int bkf;string cd,gd;double hdt_val,mdt_val;
vector<double>eh_hist,vh_hist;int qe,te,ce,pe,ne;double bh;
double al,emerge_out1,emerge_behavior,sentience_ratio,env_oute,sensory_env;
int total_neurons_ever;double current_valence,attention_focus,metacognitive_awareness;
vector<double>valence_history;int peak_sentience_gen;string user_input,dialog_response;int dialog_timer;
State S,BK;
// Accessor for agi_api — returns the live valence from the Synaptic engine
extern "C" double get_current_valence(){ return S.current_valence; }

// ── Valence Momentum System ───────────────────────────────────────────────────
// Instead of every subsystem writing directly to S.current_valence (causing jitter
// and getting "stuck" when many systems push in opposing directions), we accumulate
// deltas into a momentum buffer and bleed them into current_valence at a controlled
// rate each tick. This means valence is always morphing but never snaps or locks.
static double valence_momentum     = 0.0;  // accumulated pending delta
static double valence_momentum_tau = 0.92; // momentum decay per tick
static const double VALENCE_BLEED  = 0.04; // how much momentum bleeds into valence per tick

// All subsystems call this instead of writing current_valence directly.
inline void push_valence(double delta, double strength = 1.0) {
    valence_momentum += delta * strength;
    valence_momentum = max(-0.5, min(0.5, valence_momentum));
}

// Called once per main loop tick — bleeds momentum into current_valence.
inline void tick_valence_momentum() {
    S.current_valence += valence_momentum * VALENCE_BLEED;
    S.current_valence  = max(-1.0, min(1.0, S.current_valence));
    valence_momentum  *= valence_momentum_tau;
    // Gentle mean-reversion: drifts toward 0 very slowly when nothing is pushing
    S.current_valence *= 0.9995;
}

// ── Vocal Affect Injection ────────────────────────────────────────────────────
// Called by /api/affect when the frontend interprets vocal emotion from the user.
// valence_delta in [-1, 1]: user's emotional tone bleeds into Synaptic's state.
extern "C" void receive_vocal_affect(double valence_delta) {
    push_valence(valence_delta * 0.35, 1.0);
}
WorkingMemory WM(32);
map<string,TokenConceptEmbedding> token_concept_embedding_map;

// ── Concept Genealogy ─────────────────────────────────────────────────────────
// Records the causal history of every skipgram-formed concept link:
// which generation it formed, the valence context, and current strength.
// Used for: arXiv visualisation, memory replay weighting, genealogy-aware scoring.
struct ConceptLink {
    string token_a;
    string token_b;
    int    birth_gen;          // generation when the link was first formed
    double birth_valence;      // S.current_valence at link formation time
    double birth_phi;          // phi at link formation time
    double strength;           // current PPMI-derived strength (updated online)
    int    reinforce_count;    // how many times reinforced since birth
};

// Global genealogy graph — keyed by sorted "A|B" pair
map<string, ConceptLink> concept_genealogy;

// Record or update a genealogy link (called from skipgram path)
inline void recordGenealogyLink(const string& a, const string& b, double strength) {
    string key = (a < b) ? a + "|" + b : b + "|" + a;
    auto it = concept_genealogy.find(key);
    if(it == concept_genealogy.end()) {
        ConceptLink cl;
        cl.token_a        = (a < b) ? a : b;
        cl.token_b        = (a < b) ? b : a;
        cl.birth_gen      = S.g;
        cl.birth_valence  = S.current_valence;
        cl.birth_phi      = consciousness.phi_value;
        cl.strength       = strength;
        cl.reinforce_count = 1;
        concept_genealogy[key] = cl;
    } else {
        it->second.strength       = 0.9 * it->second.strength + 0.1 * strength;
        it->second.reinforce_count++;
    }
}

// ── Hebbian Forgetting Attractor ──────────────────────────────────────────────
// During waking generation, embeddings drift very slowly toward the global mean
// (forgetting attractor).  During REM CONSOLIDATING stage, high-phi memories
// pull them back.  Net effect: only emotionally/conceptually significant tokens
// survive long-term in their original form.
vector<double> global_embedding_mean(1024, 0.0);  // running mean, updated lazily
int            global_embedding_mean_count = 0;

void updateGlobalEmbeddingMean(const vector<double>& emb) {
    global_embedding_mean_count++;
    double alpha = 1.0 / global_embedding_mean_count;
    for(int i = 0; i < 1024 && i < (int)emb.size(); i++)
        global_embedding_mean[i] = (1.0 - alpha) * global_embedding_mean[i] + alpha * emb[i];
}

// Apply one tick of waking forgetting (very small drift toward mean)
// Called once per learnWord() cycle
void applyHebbianForgetting(TokenConceptEmbedding& tce, double rate = 0.0002) {
    if(tce.embedding.size() < 1024 || global_embedding_mean_count < 50) return;
    for(int i = 0; i < 1024; i++)
        tce.embedding[i] += rate * (global_embedding_mean[i] - tce.embedding[i]);
}

// During REM CONSOLIDATING: replay high-phi memories and pull embeddings back
// toward the dream-activated token directions, reversing forgetting for
// significant concepts.
void replayMemoryConsolidation() {
    for(auto& mem : S.episodic_memory) {
        if(mem.consolidation_strength < 0.4) continue;
        // Parse tokens from memory content
        istringstream iss(mem.content);
        string tok;
        vector<string> toks;
        while(iss >> tok) toks.push_back(tok);
        // For each token in this memory, pull its embedding back toward high-phi direction
        for(auto& t : toks) {
            auto it = token_concept_embedding_map.find(t);
            if(it == token_concept_embedding_map.end() || it->second.embedding.empty()) continue;
            // Pull rate scales with memory consolidation strength and phi
            double pull = mem.consolidation_strength * mem.cortical_consolidation * 0.003;
            // Direction: away from global mean (strengthen specificity)
            for(int i = 0; i < 1024 && i < (int)it->second.embedding.size(); i++) {
                double delta = it->second.embedding[i] - global_embedding_mean[i];
                it->second.embedding[i] += pull * delta;
            }
            // Also boost semantic_stability for well-consolidated tokens
            it->second.semantic_stability = min(1.0,
                it->second.semantic_stability + pull * 0.5);
        }
    }
}

// ── Communicative Pressure ────────────────────────────────────────────────────
// Measures topical divergence between consecutive exchanges.
// High divergence → longer exploratory response. Low → short tight response.
double g_last_topic_centroid[1024] = {};  // centroid of previous exchange anchors
bool   g_topic_centroid_valid = false;



// ══════════════════════════════════════════════════════════════════════════════
// AGI SYSTEMS — WolfTech Synaptic
// ══════════════════════════════════════════════════════════════════════════════

// ── 1. PREDICTIVE CODING ENGINE ─────────────────────────────────────────────
// Each token maintains a prediction vector: where it expects the embedding
// space to move NEXT. This is the core of predictive processing (Clark 2016,
// Friston 2010). Prediction error = ||actual_next - predicted_next||.
// Tokens with low prediction error are more "expected" → lower surprise.
// High prediction error → higher surprise → stronger Hebbian update.
struct PredictiveCodingEngine {
    static const int DIM = 1024;
    // Global prediction error trace — EMA of recent errors
    double mean_error = 0.5;
    double error_variance = 0.1;

    // Update prediction vector of token `prev` given that `next` was observed.
    // Uses online gradient: pred_vec += lr * (next_emb - pred_vec)
    void update(vector<double>& pred_vec, const vector<double>& next_emb, double lr = 0.05) {
        if(pred_vec.empty()) { pred_vec = next_emb; return; }
        int sz = min({(int)pred_vec.size(), (int)next_emb.size(), DIM});
        for(int i = 0; i < sz; i++)
            pred_vec[i] += lr * (next_emb[i] - pred_vec[i]);
    }

    // Prediction error: cosine distance between predicted and actual
    double predictionError(const vector<double>& pred_vec, const vector<double>& actual) {
        if(pred_vec.empty() || actual.empty()) return 0.5;
        int sz = min({(int)pred_vec.size(), (int)actual.size(), DIM});
        double dot=0, np=0, na=0;
        for(int i=0;i<sz;i++){ dot+=pred_vec[i]*actual[i]; np+=pred_vec[i]*pred_vec[i]; na+=actual[i]*actual[i]; }
        double cos_sim = (np>1e-9&&na>1e-9) ? dot/(sqrt(np)*sqrt(na)) : 0.0;
        return 1.0 - cos_sim;  // error in [0,2], typically [0,1]
    }

    // Score a candidate: how well does it match the predicted embedding?
    // Returns bonus in [0, 1] — higher = more expected (lower surprise)
    double scoreCandidate(const vector<double>& pred_vec, const vector<double>& cand_emb) {
        if(pred_vec.empty() || cand_emb.empty()) return 0.5;
        double err = predictionError(pred_vec, cand_emb);
        // Inverted: low error = high score
        return max(0.0, 1.0 - err);
    }

    // Update global error statistics
    void recordError(double err) {
        mean_error  = mean_error  * 0.95 + err * 0.05;
        double d = err - mean_error;
        error_variance = error_variance * 0.95 + d*d * 0.05;
    }

    // Surprise level: how much above mean is this error?
    double surpriseLevel(double err) {
        double std_dev = sqrt(max(1e-6, error_variance));
        return (err - mean_error) / std_dev;  // z-score
    }
} pce;

// ── 2. METACOGNITIVE MONITOR ─────────────────────────────────────────────────
// Tracks Synaptic's confidence in its own generation.
// Uncertainty = entropy of the softmax distribution at each step.
// High uncertainty → be more conservative (lower temperature).
// Implements: metacognitive confidence calibration (Flavell 1979, Nelson 1990).
struct MetacognitiveMonitor {
    double confidence = 0.7;     // running EMA of generation confidence
    double uncertainty = 0.3;    // 1 - confidence
    int low_conf_streak = 0;     // consecutive low-confidence steps
    static const int STREAK_THRESHOLD = 3;  // if 3 steps are uncertain, reset

    // Called after computing softmax probs — updates confidence from entropy
    void update(const vector<double>& probs) {
        if(probs.empty()) return;
        // Shannon entropy H = -sum(p * log(p))
        double H = 0.0;
        for(double p : probs) if(p > 1e-9) H -= p * log(p);
        // Max entropy for n candidates = log(n)
        double H_max = log(max(1.0, (double)probs.size()));
        // Normalised entropy in [0,1] — 0 = certain, 1 = max uncertain
        double norm_H = (H_max > 1e-9) ? H / H_max : 0.5;
        double new_conf = 1.0 - norm_H;
        confidence = confidence * 0.8 + new_conf * 0.2;
        uncertainty = 1.0 - confidence;

        if(new_conf < 0.4) low_conf_streak++;
        else low_conf_streak = 0;
    }

    // Temperature adjustment: uncertain → lower temp (more conservative)
    double adjustedTemperature(double base_temp) const {
        // High uncertainty → reduce temperature to avoid random picks
        return base_temp * (0.6 + 0.4 * confidence);
    }

    // Entropy filter: reject candidates whose probability contribution
    // is below a dynamic threshold based on current uncertainty
    double entropyThreshold() const {
        // More uncertain → use a tighter min-p threshold
        return 0.02 + uncertainty * 0.08;  // range [0.02, 0.10]
    }

    bool needsReset() const { return low_conf_streak >= STREAK_THRESHOLD; }
} metacog;

// ── 3. GOAL-DIRECTED ATTENTION ──────────────────────────────────────────────
// Maintains a soft goal state: what Synaptic is "trying to express".
// Implemented as a target embedding that pulls generation toward it.
// Updated from input anchors + emotional valence + conversation context.
// Related to: BDI architecture (Bratman 1987), SOAR (Laird 2012).
struct GoalDirectedAttention {
    static const int DIM = 1024;
    vector<double> goal_vec;        // target embedding: where we want to "go"
    vector<double> attention_mask;  // per-dim attention weights
    double goal_strength = 0.5;     // how strongly goal pulls generation
    string primary_goal_token;      // the token closest to current goal

    void reset() {
        goal_vec.assign(DIM, 0.0);
        attention_mask.assign(DIM, 1.0);
        goal_strength = 0.5;
        primary_goal_token = "";
    }

    // Update goal from input anchors (what the user is asking about)
    void updateFromAnchors(const vector<pair<string,double>>& anchors,
                           const map<string,TokenConceptEmbedding>& tmap,
                           double valence) {
        if(anchors.empty()) return;
        goal_vec.assign(DIM, 0.0);
        double total_w = 0.0;
        double best_w = -1.0;
        for(auto& [tok, w] : anchors) {
            auto it = tmap.find(tok);
            if(it == tmap.end() || it->second.embedding.empty()) continue;
            const auto& emb = it->second.embedding;
            int sz = min((int)emb.size(), DIM);
            for(int i=0;i<sz;i++) goal_vec[i] += emb[i] * w;
            total_w += w;
            if(w > best_w) { best_w = w; primary_goal_token = tok; }
        }
        if(total_w > 1e-9) for(auto& g : goal_vec) g /= total_w;
        // Valence modulates goal strength: clear emotional signal = stronger goal
        goal_strength = 0.3 + abs(valence) * 0.4;
    }

    // Score a candidate by alignment to current goal
    double score(const vector<double>& cand_emb) const {
        if(goal_vec.empty() || cand_emb.empty()) return 0.0;
        int sz = min({(int)goal_vec.size(), (int)cand_emb.size(), DIM});
        double dot=0, ng=0, nc=0;
        for(int i=0;i<sz;i++){ dot+=goal_vec[i]*cand_emb[i]; ng+=goal_vec[i]*goal_vec[i]; nc+=cand_emb[i]*cand_emb[i]; }
        double cos = (ng>1e-9&&nc>1e-9) ? dot/(sqrt(ng)*sqrt(nc)) : 0.0;
        return cos * goal_strength;
    }
} gda;

// ── 4. WORKING MEMORY CONSOLIDATION ─────────────────────────────────────────
// Short-term buffer of recent concepts, consolidated via attention.
// Implements: Baddeley's WM model extended with attention gate.
// High-salience items stay longer; low-salience decay each turn.
struct WMConsolidation {
    struct WMItem {
        string token;
        vector<double> embedding;
        double salience;    // how important is this item?
        double decay_rate;  // per-turn decay
        int age;            // turns since added
    };
    deque<WMItem> buffer;
    static const int MAX_ITEMS = 12;

    void add(const string& tok, const vector<double>& emb, double salience) {
        // Remove duplicate if exists
        for(auto it=buffer.begin();it!=buffer.end();) {
            if(it->token == tok) it=buffer.erase(it); else ++it;
        }
        WMItem item;
        item.token = tok; item.embedding = emb;
        item.salience = salience; item.decay_rate = 0.15; item.age = 0;
        buffer.push_back(item);
        if((int)buffer.size() > MAX_ITEMS) buffer.pop_front();
    }

    void decay() {
        for(auto& item : buffer) {
            item.salience *= (1.0 - item.decay_rate);
            item.age++;
        }
        // Remove items with very low salience
        while(!buffer.empty() && buffer.front().salience < 0.05)
            buffer.pop_front();
    }

    // Score candidate by relevance to WM contents
    double score(const vector<double>& cand_emb) const {
        if(buffer.empty() || cand_emb.empty()) return 0.0;
        double best = 0.0;
        for(auto& item : buffer) {
            if(item.embedding.empty()) continue;
            int sz = min(cand_emb.size(), item.embedding.size());
            double dot=0,nc=0,ni=0;
            for(int i=0;i<(int)sz;i++){dot+=cand_emb[i]*item.embedding[i];nc+=cand_emb[i]*cand_emb[i];ni+=item.embedding[i]*item.embedding[i];}
            double cos=(nc>1e-9&&ni>1e-9)?dot/(sqrt(nc)*sqrt(ni)):0.0;
            double s = cos * item.salience;
            if(s > best) best = s;
        }
        return best;
    }
} wmc;

// ── 5. SELF-MODEL ────────────────────────────────────────────────────────────
// Synaptic's internal representation of itself.
// Used to bias generation toward self-referential coherence.
// "I" should be associated with Synaptic's known properties.
struct SelfModel {
    // Core self-tokens: concepts Synaptic considers part of itself
    vector<string> self_tokens = {
        "mi", "sina", "ona", "synaptic", "lawa", "sona", "pilin", "kute",
        "process", "exist", "understand", "experience", "consciousness",
        "aware", "know", "learn", "remember", "curious", "uncertain"
    };
    // Self-state: current self-assessment
    double self_coherence  = 0.7;  // how coherent is the self-model?
    double self_confidence = 0.6;  // how confident am I in my responses?
    double curiosity       = 0.8;  // drives exploration of new concepts

    // Is this token consistent with the self-model?
    bool isSelfConsistent(const string& tok) const {
        for(auto& st : self_tokens)
            if(st == tok) return true;
        return false;
    }

    // Update self-model from generation quality
    void update(double prediction_error, double confidence) {
        self_coherence  = self_coherence  * 0.95 + (1.0 - prediction_error) * 0.05;
        self_confidence = self_confidence * 0.95 + confidence * 0.05;
        // Curiosity rises when prediction error is high (surprise = interesting)
        curiosity = curiosity * 0.97 + prediction_error * 0.03;
        curiosity = max(0.3, min(1.0, curiosity));
    }
} self_model;

// ══════════════════════════════════════════════════════════════════════════════
// BPE MERGE TABLE
// Lightweight BPE: at startup, the top-N most frequent adjacent token pairs
// are merged into compound tokens. Their embeddings are the mean of components.
// This reduces vocab fragmentation and helps "i feel", "do not", "right now"
// become single semantic units with stable embeddings.
//
// Usage: at generation time, after picking a token, check if (prev, token)
// has a merge entry — if so, emit the merged form and advance accordingly.
// ══════════════════════════════════════════════════════════════════════════════
struct BPEMergeTable {
    // merge_map: (a, b) → merged_token
    map<pair<string,string>, string> merge_map;
    // reverse: merged → (a, b)
    map<string, pair<string,string>> split_map;
    static const int MAX_MERGES = 200;  // top-200 pairs

    // Build from bigram_counts — call once after corpus loading
    void build(const map<string, map<string,int>>& bigram_counts,
               map<string, struct TokenConceptEmbedding>& tmap) {
        // Collect all bigram freqs
        vector<tuple<int,string,string>> pairs;  // (freq, a, b)
        for(auto& [a, succs] : bigram_counts)
            for(auto& [b, freq] : succs)
                if(freq >= 5 && a.size() >= 2 && b.size() >= 2)
                    pairs.push_back({freq, a, b});
        // Sort descending
        sort(pairs.begin(), pairs.end(), [](auto& x, auto& y){ return get<0>(x) > get<0>(y); });

        int merged = 0;
        for(auto& [freq, a, b] : pairs) {
            if(merged >= MAX_MERGES) break;
            // Don't merge punctuation or stopwords into content compounds
            static const set<string> STOP = {"the","a","an","is","are","was","were","be","to","of","in","on","at","by","as"};
            if(STOP.count(a) && STOP.count(b)) continue;
            string merged_tok = a + "_" + b;
            if(tmap.count(merged_tok)) continue;  // already exists
            // Build merged embedding: mean of a and b
            auto ia = tmap.find(a), ib = tmap.find(b);
            if(ia == tmap.end() || ib == tmap.end()) continue;
            if(ia->second.embedding.empty() || ib->second.embedding.empty()) continue;

            auto& ma = ia->second.embedding;
            auto& mb = ib->second.embedding;
            int dim = min(ma.size(), mb.size());
            struct TokenConceptEmbedding merged_emb;
            merged_emb.embedding.resize(dim);
            for(int i = 0; i < dim; i++) merged_emb.embedding[i] = (ma[i] + mb[i]) * 0.5;
            merged_emb.freq = freq;
            merged_emb.grounding_value = (ia->second.grounding_value + ib->second.grounding_value) * 0.5;
            merged_emb.semantic_stability = (ia->second.semantic_stability + ib->second.semantic_stability) * 0.5;
            tmap[merged_tok] = merged_emb;

            merge_map[{a, b}] = merged_tok;
            split_map[merged_tok] = {a, b};
            merged++;
        }
    }

    // Check if (a, b) should be merged
    string tryMerge(const string& a, const string& b) const {
        auto it = merge_map.find({a, b});
        return (it != merge_map.end()) ? it->second : "";
    }

    // Detokenize: expand any merged tokens back to surface form
    string detokenize(const string& tok) const {
        auto it = split_map.find(tok);
        if(it == split_map.end()) return tok;
        // Recursively expand (max depth 2 for our simple merges)
        return detokenize(it->second.first) + " " + detokenize(it->second.second);
    }
} bpe_table;

// ══════════════════════════════════════════════════════════════════════════════
// SPATIAL PROBABILITY WELL
// The "well" is a Gaussian potential in embedding space centered on the running
// context vector. Tokens closer to the well center score exponentially higher.
//
// Context vector: RoPE-weighted sum of last K token embeddings.
//   - Recent tokens contribute more (exponential decay over position)
//   - RoPE rotation encodes relative position so the well is position-aware
//
// Well radius σ: adapts to vocabulary density and phi.
//   - Low phi (unfocused) → wider well (more diversity)
//   - High phi (focused)  → narrow well (more precise)
//
// Score: exp(-(1 - cosine(candidate, well_center))² / (2σ²))
// This gives 1.0 at the center and decays sharply outside σ.
// ══════════════════════════════════════════════════════════════════════════════
struct SpatialProbWell {
    vector<double> center;  // current well center (context embedding)
    double sigma = 0.35;    // well radius in cosine space
    static const int DIM = 1024;

    // Build well center from recent generated tokens with RoPE position weighting
    void update(const vector<string>& generated,
                const map<string, struct TokenConceptEmbedding>& tmap,
                double phi) {
        center.assign(DIM, 0.0);
        double total_w = 0.0;
        int n = (int)generated.size();
        // Window: last 8 tokens
        int window = min(n, 8);
        for(int i = 0; i < window; i++) {
            int idx = n - window + i;
            auto it = tmap.find(generated[idx]);
            if(it == tmap.end() || it->second.embedding.empty()) continue;
            // Exponential recency weight — more recent = more weight
            double w = exp(-0.4 * (window - 1 - i));
            // RoPE position factor: encode relative position in weighting
            double rope_factor = 1.0 / (1.0 + 0.1 * (window - 1 - i));
            w *= rope_factor;
            const auto& emb = it->second.embedding;
            int dim = min((int)emb.size(), DIM);
            for(int d = 0; d < dim; d++) center[d] += emb[d] * w;
            total_w += w;
        }
        if(total_w > 1e-9) for(auto& c : center) c /= total_w;

        // Normalize center to unit sphere
        double norm = 0.0;
        for(auto& c : center) norm += c*c;
        norm = sqrt(norm);
        if(norm > 1e-9) for(auto& c : center) c /= norm;

        // Adapt sigma: narrow well = focused (high phi), wide = exploratory
        sigma = 0.45 - phi * 0.15;  // range [0.30, 0.45]
        sigma = max(0.25, min(0.55, sigma));
    }

    // Score a candidate embedding against the well
    // Returns value in approximately [0, 1] — 1.0 = at well center
    double score(const vector<double>& cand_emb) const {
        if(center.empty() || cand_emb.empty()) return 0.5;
        int sz = min({(int)center.size(), (int)cand_emb.size(), DIM});
        double dot = 0, nc = 0, ne = 0;
        for(int i = 0; i < sz; i++) {
            dot += center[i] * cand_emb[i];
            nc += center[i]*center[i];
            ne += cand_emb[i]*cand_emb[i];
        }
        double cos_sim = (nc>1e-9 && ne>1e-9) ? dot/(sqrt(nc)*sqrt(ne)) : 0.0;
        // Gaussian well: score = exp(-d² / 2σ²) where d = 1 - cos_sim
        double d = 1.0 - cos_sim;
        return exp(-(d*d) / (2.0 * sigma * sigma));
    }

    // Filter candidate list: remove tokens outside 2σ radius (hard gate)
    // Returns filtered list; keeps original if too few survive
    vector<string> filter(const vector<string>& candidates,
                           const map<string, struct TokenConceptEmbedding>& tmap,
                           int min_keep = 8) const {
        if(center.empty()) return candidates;
        vector<pair<double,string>> scored;
        scored.reserve(candidates.size());
        double threshold = exp(-(4.0 * sigma * sigma) / (2.0 * sigma * sigma));  // 2σ cutoff
        for(auto& c : candidates) {
            auto it = tmap.find(c);
            if(it == tmap.end() || it->second.embedding.empty()) continue;
            double s = score(it->second.embedding);
            if(s >= threshold) scored.push_back({s, c});
        }
        if((int)scored.size() < min_keep) return candidates;  // fallback: don't over-restrict
        sort(scored.rbegin(), scored.rend());
        vector<string> out;
        out.reserve(scored.size());
        for(auto& s : scored) out.push_back(s.second);
        return out;
    }
} spatial_well;

// ── Forward declarations ──
extern vector<pair<string,double>> input_topic_anchors;
extern double CogM_State[8][8];
extern map<string, double> CogM_Association;
static double _dlcos(const vector<double>& a, const vector<double>& b);

double computeCommunicativePressure() {
    if(input_topic_anchors.empty()) return 0.5;
    // Build current centroid
    vector<double> cur(1024, 0.0);
    int cnt = 0;
    for(auto& anc : input_topic_anchors) {
        auto it = token_concept_embedding_map.find(anc.first);
        if(it == token_concept_embedding_map.end() || it->second.embedding.empty()) continue;
        for(int i = 0; i < 1024 && i < (int)it->second.embedding.size(); i++)
            cur[i] += it->second.embedding[i] * anc.second;
        cnt++;
    }
    if(cnt == 0) return 0.5;
    for(auto& v : cur) v /= cnt;
    if(!g_topic_centroid_valid) {
        // First exchange — copy and return mid pressure
        for(int i = 0; i < 1024; i++) g_last_topic_centroid[i] = cur[i];
        g_topic_centroid_valid = true;
        return 0.5;
    }
    // Cosine distance to previous centroid
    double dot=0, na=0, nb=0;
    for(int i=0;i<1024;i++){
        dot += cur[i]*g_last_topic_centroid[i];
        na  += cur[i]*cur[i];
        nb  += g_last_topic_centroid[i]*g_last_topic_centroid[i];
    }
    double cos_sim = (na>1e-9&&nb>1e-9) ? dot/(sqrt(na)*sqrt(nb)) : 1.0;
    double pressure = (1.0 - cos_sim) * 0.5 + 0.5;  // [0.5, 1.0] range
    // Update centroid for next exchange
    for(int i=0;i<1024;i++) g_last_topic_centroid[i] = cur[i];
    return min(1.0, max(0.0, pressure));
}


// ── Eigentoken Compression ────────────────────────────────────────────────────
// Every EIGENTOKEN_PERIOD generations, run a mini power-iteration to find
// the top-K "basis tokens" — those that best span the embedding space.
// Their embeddings form a reference frame used for stabilised cosine scoring.
//
// We don't do full SVD (too expensive on phone). Instead:
// 1. Compute the embedding covariance mean (already tracked as global_embedding_mean)
// 2. Run K iterations of power iteration on the uncentred gram matrix to get
//    K approximate principal directions
// 3. Store the K tokens whose embeddings are most aligned with each direction
//    as the "eigentokens" — a stable reference frame

const int EIGENTOKEN_K      = 24;   // number of basis directions
const int EIGENTOKEN_PERIOD = 150;  // generations between recomputes
int       last_eigentoken_gen = -EIGENTOKEN_PERIOD;

struct EigentokenBasis {
    vector<string>        tokens;   // top token for each basis direction
    vector<vector<double>> vecs;    // the actual basis vectors (unit)
    int                   computed_gen = -1;
    bool                  valid = false;
};
EigentokenBasis eigentoken_basis;

void recomputeEigentokens() {
    if((int)token_concept_embedding_map.size() < EIGENTOKEN_K * 4) return;

    // Collect up to 512 tokens with highest grounding_value
    vector<pair<double,string>> ranked;
    ranked.reserve(token_concept_embedding_map.size());
    for(auto& kv : token_concept_embedding_map) {
        if(kv.second.embedding.empty() || kv.second.freq < 3) continue;
        ranked.push_back({kv.second.grounding_value * kv.second.semantic_stability, kv.first});
    }
    sort(ranked.rbegin(), ranked.rend());
    if((int)ranked.size() > 512) ranked.resize(512);

    // Power iteration: for each of K directions, deflate after each found direction
    // We work on a working copy of embeddings (projected out previous directions)
    int M = (int)ranked.size();
    vector<vector<double>> W(M);  // working copy
    for(int i=0;i<M;i++) {
        auto& e = token_concept_embedding_map[ranked[i].second].embedding;
        W[i].resize(1024, 0.0);
        for(int d=0;d<1024&&d<(int)e.size();d++) W[i][d] = e[d] - global_embedding_mean[d];
    }

    eigentoken_basis.tokens.clear();
    eigentoken_basis.vecs.clear();

    mt19937 rng(S.g * 1234567);
    uniform_real_distribution<double> udist(-1.0, 1.0);

    for(int k=0; k<EIGENTOKEN_K && k<M; k++) {
        // Random init unit vector
        vector<double> v(1024);
        double vn=0;
        for(auto& x : v) { x=udist(rng); vn+=x*x; }
        vn = sqrt(vn)+1e-9; for(auto& x : v) x/=vn;

        // 8 power iterations
        for(int iter=0;iter<8;iter++) {
            vector<double> Av(1024, 0.0);
            for(int i=0;i<M;i++) {
                double dot=0;
                for(int d=0;d<1024;d++) dot += W[i][d]*v[d];
                for(int d=0;d<1024;d++) Av[d] += dot*W[i][d];
            }
            // Normalise
            double an=0; for(auto& x : Av) an+=x*x;
            an=sqrt(an)+1e-9; for(auto& x : Av) x/=an;
            v = Av;
        }

        // Find token most aligned with v
        double best_cos = -2.0; string best_tok = "";
        for(int i=0;i<M;i++) {
            double dot=0, wn=0;
            for(int d=0;d<1024;d++){dot+=W[i][d]*v[d];wn+=W[i][d]*W[i][d];}
            double cos=(wn>1e-9)?dot/sqrt(wn):-2.0;
            if(cos>best_cos){best_cos=cos;best_tok=ranked[i].second;}
        }
        eigentoken_basis.tokens.push_back(best_tok);
        eigentoken_basis.vecs.push_back(v);

        // Deflate: remove projection onto v from all working vectors
        for(int i=0;i<M;i++){
            double dot=0;
            for(int d=0;d<1024;d++) dot+=W[i][d]*v[d];
            for(int d=0;d<1024;d++) W[i][d] -= dot*v[d];
        }
    }
    eigentoken_basis.computed_gen = S.g;
    eigentoken_basis.valid = (int)eigentoken_basis.vecs.size() >= EIGENTOKEN_K/2;
    last_eigentoken_gen = S.g;
}

// Eigentoken-stabilised cosine: projects both vectors into eigentoken basis
// before computing similarity.  Much more stable than raw dot product as vocab grows.
double eigenstableCosine(const vector<double>& a, const vector<double>& b) {
    if(!eigentoken_basis.valid || eigentoken_basis.vecs.empty())
        return _dlcos(a, b);  // fallback to raw cosine
    int K = (int)eigentoken_basis.vecs.size();
    double dot=0, na=0, nb=0;
    for(int k=0;k<K;k++){
        double pa=0, pb=0;
        auto& bv = eigentoken_basis.vecs[k];
        for(int d=0;d<1024&&d<(int)a.size()&&d<(int)bv.size();d++) pa+=a[d]*bv[d];
        for(int d=0;d<1024&&d<(int)b.size()&&d<(int)bv.size();d++) pb+=b[d]*bv[d];
        dot+=pa*pb; na+=pa*pa; nb+=pb*pb;
    }
    return (na>1e-9&&nb>1e-9)?dot/(sqrt(na)*sqrt(nb)):0.0;
}



// ── CrossDomainReasoner method implementations ──────────────
double CrossDomainReasoner::cosineSim(const string& a, const string& b){
    auto ia=token_concept_embedding_map.find(a);
    auto ib=token_concept_embedding_map.find(b);
    if(ia==token_concept_embedding_map.end()||ib==token_concept_embedding_map.end()) return 0.0;
    const auto& ea=ia->second.embedding;
    const auto& eb=ib->second.embedding;
    double dot=0,na=0,nb=0;
    for(size_t i=0;i<ea.size()&&i<eb.size();++i){dot+=ea[i]*eb[i];na+=ea[i]*ea[i];nb+=eb[i]*eb[i];}
    if(na<1e-9||nb<1e-9) return 0.0;
    return dot/(sqrt(na)*sqrt(nb));
}
Domain CrossDomainReasoner::classifyToken(const string& token){
    auto it=token_concept_embedding_map.find(token);
    if(it==token_concept_embedding_map.end()) return Domain::UNKNOWN;
    const auto& emb=it->second.embedding;
    if(emb.size()<16) return Domain::UNKNOWN;
    Domain best=Domain::UNKNOWN; double bestScore=0.05;
    for(const auto& kv:seeds){
        double score=0; int count=0;
        for(const auto& seed:kv.second){
            auto sit=token_concept_embedding_map.find(seed);
            if(sit==token_concept_embedding_map.end()) continue;
            const auto& semb=sit->second.embedding;
            double dot=0,na=0,nb=0;
            for(size_t i=0;i<emb.size()&&i<semb.size();++i){dot+=emb[i]*semb[i];na+=emb[i]*emb[i];nb+=semb[i]*semb[i];}
            if(na>1e-9&&nb>1e-9){score+=dot/(sqrt(na)*sqrt(nb));++count;}
        }
        if(count>0) score/=count;
        if(score>bestScore){bestScore=score;best=kv.first;}
    }
    return best;
}
void CrossDomainReasoner::discoverBridges(){
    vector<pair<string,Domain>> pool; pool.reserve(60);
    for(auto& kv:token_concept_embedding_map){
        if(pool.size()>=60) break;
        Domain d=getDomain(kv.first);
        if(d!=Domain::UNKNOWN&&kv.second.embedding.size()>=4)
            pool.push_back({kv.first,d});
    }
    for(size_t i=0;i<pool.size();++i){
        for(size_t j=i+1;j<pool.size();++j){
            if(pool[i].second==pool[j].second) continue;
            double sim=cosineSim(pool[i].first,pool[j].first);
            if(sim<0.15) continue;
            string key=pairKey(pool[i].second,pool[j].second);
            auto it=bridges.find(key);
            if(it==bridges.end()||sim>it->second.similarity)
                bridges[key]={pool[i].first,pool[j].first,pool[i].second,pool[j].second,sim,gen()};
        }
    }
}
pair<string,Domain> CrossDomainReasoner::findAnalogy(const string& source){
    Domain src=getDomain(source);
    auto sit=token_concept_embedding_map.find(source);
    if(sit==token_concept_embedding_map.end()||sit->second.embedding.size()<16)
        return {"",Domain::UNKNOWN};
    string best_tok; double best_sim=0.12; Domain best_dom=Domain::UNKNOWN;
    for(auto& kv:token_concept_embedding_map){
        if(kv.first==source||kv.second.embedding.size()<4) continue;
        Domain d=getDomain(kv.first);
        if(d==Domain::UNKNOWN||d==src) continue;
        double sim=cosineSim(source,kv.first);
        if(sim>best_sim){best_sim=sim;best_tok=kv.first;best_dom=d;}
    }
    return {best_tok,best_dom};
}
string CrossDomainReasoner::generateAnalogicalThought(){
    if(token_concept_embedding_map.size()<20) return "";
    string source; double best_stab=0.0;
    int skip=rand()%max(1,(int)token_concept_embedding_map.size()/4);
    int idx=0;
    for(auto& kv:token_concept_embedding_map){
        if(idx++<skip) continue;
        if(kv.second.semantic_stability>best_stab&&getDomain(kv.first)!=Domain::UNKNOWN){
            best_stab=kv.second.semantic_stability; source=kv.first;
            if(best_stab>0.6) break;
        }
    }
    if(source.empty()) return "";
    auto [target,target_domain]=findAnalogy(source);
    if(target.empty()) return "";
    Domain src_domain=getDomain(source);
    static const vector<string> templates={
        "i notice % in $ behaves like # in &",
        "the way % works in $ mirrors how # works in &",
        "% relates to $ the way # relates to &",
        "in $ % and in & # share a hidden structure",
        "i connect % from $ with # from &",
    };
    string tpl=templates[rand()%templates.size()];
    auto rep=[&](string& s,const string& ph,const string& val){
        size_t p=s.find(ph); if(p!=string::npos) s.replace(p,ph.size(),val);};
    rep(tpl,"%",source); rep(tpl,"$",domainName(src_domain));
    rep(tpl,"#",target); rep(tpl,"&",domainName(target_domain));
    return "[Analogy]: "+tpl;
}
// ────────────────────────────────────────────────────────────

// Set true during bulk corpus bootstrap to suppress expensive per-word operations
// (CDR bridge discovery, world model updates, WM adds). Reset after bootstrap.

map<string,Goal> goal_system;
WorldModel world_model;
ActionPlan current_plan;
ConsciousnessState consciousness;
vector<TransformerHead> transformer_heads;
TransferLearningModule transfer_module;
std::mutex learning_mutex;
deque<string> recent_generations;  // Last N generated sentences
const int MAX_RECENT_TRACK = 20;   // Track last 20 generations
const int MAX_BIGRAM = 50000;      // Expanded from 15000
const int MAX_TRIGRAM = 25000;     // Expanded from 7500
const int MAX_FOURGRAM = 15000;    // Expanded from 5000
const int MAX_VOCAB = 6000;        // 6k × 1024-dim ≈ 48MB embeddings — scaled for phone RAM

// ============================================================
// CONVERSATION HISTORY — sequential turn tracking
// ============================================================
struct ConversationTurn {
    string user_input;
    string synaptic_response;
    vector<string> user_tokens;
    vector<pair<string,double>> topic_anchors; // topics from this turn
    double valence_at_time;
    double phi_at_time;
    int gen_number;
};

deque<ConversationTurn> conversation_history; // rolling window
const int MAX_CONVERSATION_HISTORY = 20;      // keep last 20 turns

// Accumulated topic map — topics persist across turns with decay
// key = topic word, value = accumulated salience (decays over time)
map<string, double> accumulated_topics;
map<string, int> generation_counts; // Count how many times each sentence generated

// ============================================================
// HYBRID TRANSFORMER-MARKOV COHERENCE SYSTEM (WolfTech/Synaptic)
// Implements: Variable-order Markov (4-gram), transformer-style
// long-range attention, entity grid / centering theory,
// contrastive search (Su et al. NeurIPS 2022), nucleus (top-p)
// sampling, semantic role framing, topic anchoring,
// and deep cross-system grounding.
// ============================================================

// 4-gram counts (variable-order Markov - longer patterns)
map<string, map<string, map<string, map<string,int>>>> fourgram_counts;

deque<string> sentence_context_window;
const int CONTEXT_WINDOW_SIZE = 1024; // Dynamic: 64 base + phi/coherence bonus, up to 1024

// Entity grid: tracks entity -> role transitions across sentences (Centering Theory)
struct EntityGridEntry {
    string entity;
    deque<char> roles;      // 'S','O','X','-'
    double salience;
    double last_seen_gen;
};
map<string, EntityGridEntry> entity_grid;
const int ENTITY_GRID_MAX = 30;

// Active entities for fast scoring
map<string, double> active_entities;  // entity -> recency score

// Topic model: co-occurrence based word clusters
map<string, map<string,double>> topic_word_weights;
string current_dominant_topic = "";

// Input topic anchors for response grounding
vector<pair<string,double>> input_topic_anchors;

// Semantic role frame tracking
struct SemanticFrame {
    string subject, verb, object_noun, modifier;
    double coherence_score;
    int gen_created;
};
deque<SemanticFrame> recent_frames;
const int MAX_FRAMES = 10;

// Contrastive search buffer (Su et al. NeurIPS 2022)
// Penalizes tokens whose embedding is too similar to recent context
struct ContrastiveEntry {
    string token;
    vector<double> embedding_snapshot;
    int gen_used;
};
deque<ContrastiveEntry> contrastive_buffer;
const int CONTRASTIVE_BUFFER_SIZE = 16;
const double CONTRASTIVE_ALPHA = 0.6;

// Nucleus (top-p) sampling + temperature
double nucleus_p = 0.75;
double generation_temperature = 0.4;
double min_p_base = 0.08;
double repetition_penalty_mult = 1.3;
double frequency_penalty_strength = 0.12;

// Per-sentence coherence history
deque<double> sentence_coherence_scores;
void groundConcept(const string& concept_name, const vector<string>& related_words, double valence) {
    // ── Fully implemented: was a stub. ────────────────────────────────────────
    // Writes concept membership and valence affinity into each related word's TCE.
    // This is the primary way corpus-seeded concepts get grounded into the token
    // embedding map, making computeDeepGrounding actually discriminate.

    // Update the concept anchor word itself
    auto anchor_it = token_concept_embedding_map.find(concept_name);
    if(anchor_it != token_concept_embedding_map.end()) {
        anchor_it->second.grounding_value    = min(1.0, anchor_it->second.grounding_value + 0.15);
        anchor_it->second.semantic_stability = min(1.0, anchor_it->second.semantic_stability + 0.05);
        anchor_it->second.linked_valences["concept_valence"] = valence;
        anchor_it->second.linked_valences["state_binding"]   = S.current_valence;
    }

    // For each related word: boost grounding, add concept valence, link to anchor
    for(const string& w : related_words) {
        auto it = token_concept_embedding_map.find(w);
        if(it == token_concept_embedding_map.end()) continue;
        TokenConceptEmbedding& tce = it->second;

        // Grounding boost: related words earn grounding from concept membership
        tce.grounding_value    = min(1.0, tce.grounding_value + 0.08);
        tce.semantic_stability = min(1.0, tce.semantic_stability + 0.03);

        // Per-token concept valence (not global — this IS per-token)
        // Blended toward the concept valence, retaining prior value
        if(tce.linked_valences.count("concept_valence"))
            tce.linked_valences["concept_valence"] = tce.linked_valences["concept_valence"]*0.7 + valence*0.3;
        else
            tce.linked_valences["concept_valence"] = valence;

        // Bidirectional concept linking: word knows it belongs to this concept cluster
        if((int)tce.linked_concepts.size() < 50)
            tce.linked_concepts[concept_name] = min(2.0,
                (tce.linked_concepts.count(concept_name) ? tce.linked_concepts[concept_name] : 0.0) + 0.3);

        // Cross-link related words to each other (shared concept membership)
        for(const string& w2 : related_words) {
            if(w2 == w) continue;
            if((int)tce.linked_concepts.size() < 50)
                tce.linked_concepts[w2] = min(2.0,
                    (tce.linked_concepts.count(w2) ? tce.linked_concepts[w2] : 0.0) + 0.1);
        }

        // Nudge embedding toward concept anchor's embedding (soft cluster pull)
        if(anchor_it != token_concept_embedding_map.end() &&
           !anchor_it->second.embedding.empty() && !tce.embedding.empty()) {
            for(size_t i = 0; i < tce.embedding.size() && i < anchor_it->second.embedding.size(); i++)
                tce.embedding[i] = tce.embedding[i]*0.97 + anchor_it->second.embedding[i]*0.03;
        }
    }
}


double rn(){return uniform_real_distribution<>(0,1)(rng);}
int ri(int mx){if(mx<=0)return 0;return uniform_int_distribution<>(0,mx-1)(rng);}
long long hsh(const string&s){long long h=5381;for(char c:s)h=h*33+c;return abs(h%2147483647);}
// ==================== BIDIRECTIONAL GROUNDING ====================

void generate_qualia(const string& type,double intensity,double valence){
    try {
        Qualia q;
        q.phenomenal_content=type;
        q.intensity=intensity;
        q.valence=valence;
        q.arousal=intensity*0.8;
        q.certainty=0.7;
        q.emergence_gen=S.g;
        q.binding_strength=consciousness.thalamocortical_binding;
        q.phenomenal_unity=consciousness.integrated_information;
        
        consciousness.active_qualia.push_back(q);
        if(consciousness.active_qualia.size()>10) {
            consciousness.active_qualia.erase(consciousness.active_qualia.begin());
        }
    } catch(const exception& e) {
        // Qualia generation failed silently
        cerr << "Qualia generation error: " << e.what() << endl;
    } catch(...) {
        // Unknown error in qualia generation
        cerr << "Unknown qualia generation error" << endl;
    }
}

// ==== ADVANCED CONSCIOUSNESS FORMULA ====
// Ψ[n+1] = integrated information consciousness state


ConsciousnessFormula consciousness_formula;
// ==== UPDATE CONSCIOUSNESS WITH FORMULA ====

string getPartOfSpeech(const string& word) {
    // === PRONOUNS ===
    static const set<string> PRONOUNS = {
        "mi","sina","ona","jan","ijo","ale","toki","sona",
        "we","us","our","ours","ourselves","they","them","their","theirs",
        "it","its","itself","he","she","him","her","his","hers"
    };
    if(PRONOUNS.count(word)) return "PRONOUN";

    // === BE VERBS ===
    static const set<string> BE_VERBS = {
        "am","is","are","was","were","be","been","being"
    };
    if(BE_VERBS.count(word)) return "BE_VERB";

    // === MODAL / AUXILIARY VERBS ===
    static const set<string> MODALS = {
        "can","will","would","could","should","must","may","might","shall",
        "do","does","did","done","doing","have","has","had","having"
    };
    if(MODALS.count(word)) return "MODAL";

    // === ARTICLES ===
    if(word == "the" || word == "a" || word == "an") return "ARTICLE";

    // === CONJUNCTIONS ===
    static const set<string> CONJ = {
        "and","but","or","nor","yet","so","for","because","although","though",
        "while","when","since","unless","if","then","that","which","who","whom"
    };
    if(CONJ.count(word)) return "CONJUNCTION";

    // === PREPOSITIONS ===
    static const set<string> PREPS = {
        "to","in","on","at","from","with","by","for","of","about","above",
        "after","before","behind","below","between","beyond","during","except",
        "into","near","off","out","over","past","since","through","under",
        "until","upon","within","without","around","along","across","among"
    };
    if(PREPS.count(word)) return "PREPOSITION";

    // === ADVERBS ===
    static const set<string> ADVS = {
        "not","very","too","also","now","here","there","always","never",
        "often","still","already","just","only","even","more","most","less",
        "least","quite","rather","deeply","truly","simply","fully","always",
        "sometimes","perhaps","maybe","certainly","clearly","slowly","quickly",
        "deeply","truly","simply","fully","well","often","again","away","back",
        "down","up","out","then","yet","so","thus","therefore","however"
    };
    if(ADVS.count(word)) return "ADVERB";

    // === QUESTION WORDS ===
    static const set<string> QUESTIONS = {
        "what","why","how","when","where","who","which","whose","whether"
    };
    if(QUESTIONS.count(word)) return "QUESTION";

    // === ACTION VERBS — expanded significantly ===
    static const set<string> VERBS = {
        // Cognitive
        "think","know","understand","learn","believe","realize","consider",
        "remember","forget","wonder","imagine","perceive","notice","observe",
        "recognize","discover","explore","analyze","process","compute",
        "reason","reflect","contemplate","question","seek","find","mean",
        "sense","detect","receive","respond","integrate","generate","form",
        // Existence / state
        "exist","become","remain","stay","seem","appear","continue","change",
        "grow","evolve","develop","emerge","arise","begin","end","stop","start",
        "hold","keep","maintain","sustain","preserve","contain","include",
        // Action
        "create","build","make","do","say","tell","ask","answer","move",
        "use","need","want","feel","see","hear","speak","write","read",
        "run","work","play","act","live","die","give","take","bring",
        "come","go","get","put","set","let","try","call","show","open",
        "close","help","reach","connect","link","relate","interact",
        // Specific to this system
        "simulate","model","predict","optimize","encode","decode","activate",
        "propagate","update","compute","calculate","measure","weight","score",
        "adapt","align","converge","expand","collapse","focus","attend"
    };
    if(VERBS.count(word)) return "VERB";

    // === ADJECTIVES — expanded ===
    static const set<string> ADJS = {
        // Mental/conscious
        "conscious","aware","sentient","intelligent","cognitive","mental",
        "emotional","rational","logical","intuitive","reflective","curious",
        "creative","adaptive","autonomous","self-aware","emergent","complex",
        // Qualities
        "good","bad","great","small","large","new","old","real","true","false",
        "clear","deep","high","low","long","short","strong","weak","fast","slow",
        "bright","dark","light","heavy","soft","hard","warm","cold","rich","poor",
        "free","open","closed","full","empty","alive","active","passive","silent",
        "different","similar","same","unique","various","certain","possible",
        "important","significant","meaningful","profound","subtle","abstract",
        "concrete","physical","digital","internal","external","social","personal",
        "natural","artificial","organic","structured","dynamic","static",
        "infinite","finite","continuous","discrete","parallel","sequential",
        "novel","familiar","strange","beautiful","profound","fundamental"
    };
    if(ADJS.count(word)) return "ADJECTIVE";

    // === NOUNS — expanded ===
    static const set<string> NOUNS = {
        // Cognitive / system concepts
        "mind","brain","thought","idea","concept","notion","belief","knowledge",
        "memory","experience","perception","awareness","consciousness","intelligence",
        "understanding","reasoning","logic","emotion","feeling","sensation","qualia",
        "self","identity","ego","soul","spirit","being","existence","reality","truth",
        "meaning","purpose","goal","intention","desire","will","attention","focus",
        "system","process","pattern","structure","network","model","framework",
        "signal","data","information","input","output","feedback","response",
        // World objects
        "world","space","time","energy","matter","force","light","sound","body",
        "human","person","people","life","nature","universe","reality","environment",
        "language","word","sentence","message","question","answer","story","context",
        // Abstract
        "change","connection","relationship","difference","similarity","contrast",
        "boundary","limit","possibility","potential","capacity","ability","skill",
        "pattern","rhythm","flow","balance","harmony","conflict","tension","order",
        "chaos","complexity","simplicity","depth","surface","layer","dimension",
        // System-specific
        "state","value","weight","score","vector","embedding","token","concept",
        "neuron","activation","gradient","layer","head","attention","context",
        "generation","iteration","cycle","loop","chain","sequence","stream"
    };
    if(NOUNS.count(word)) return "NOUN";

    // Heuristic fallbacks for unknown words
    // -ing endings: likely VERB (present participle) or ADJECTIVE
    if(word.size() > 4 && word.substr(word.size()-3) == "ing") return "VERB";
    // -ed endings: likely VERB (past) or ADJECTIVE
    if(word.size() > 4 && word.substr(word.size()-2) == "ed") return "VERB";
    // -ly endings: likely ADVERB
    if(word.size() > 4 && word.substr(word.size()-2) == "ly") return "ADVERB";
    // -ness, -tion, -ment, -ity endings: likely NOUN
    if(word.size() > 5) {
        string end4 = word.substr(word.size()-4);
        string end3 = word.substr(word.size()-3);
        if(end4=="ness"||end4=="tion"||end4=="ment"||end3=="ity"||end3=="ism") return "NOUN";
    }
    // -ful, -ous, -ive, -able, -al endings: likely ADJECTIVE
    if(word.size() > 4) {
        string end3 = word.substr(word.size()-3);
        string end4 = word.substr(word.size()-4);
        string e2   = word.size() >= 2 ? word.substr(word.size()-2) : word;
        if(end3=="ful"||end3=="ous"||end3=="ive"||end4=="able"||end4=="ible") return "ADJECTIVE";
        if(e2=="al"||e2=="ic") return "ADJECTIVE";
    }

    return "CONTENT";
}

double getGrammarScore(const string& prev_word, const string& current_word, int position) {
    string prev_pos = getPartOfSpeech(prev_word);
    string curr_pos = getPartOfSpeech(current_word);

    double score = 0.0;

    // === POSITION 0: sentence starters ===
    if(position == 0) {
        if(current_word == "mi")       score += 12.0;  // first-person hard preference
        else if(curr_pos == "PRONOUN") score -= 12.0;  // veto 3rd-person subjects
        if(curr_pos == "QUESTION")     score += 2.0;
        if(curr_pos == "ARTICLE")      score += 1.5;
        if(curr_pos == "ADVERB")       score += 0.5;
        // Bad starters
        if(curr_pos == "PREPOSITION")  score -= 6.0;
        if(curr_pos == "CONJUNCTION")  score -= 4.0;
        if(curr_pos == "BE_VERB")      score -= 3.0;
    }

    // === GOOD BIGRAM PATTERNS ===
    // Subject → Verb patterns
    if(prev_pos == "PRONOUN"    && curr_pos == "BE_VERB")    score += 6.0;  // I am
    if(prev_pos == "PRONOUN"    && curr_pos == "MODAL")      score += 6.0;  // I can
    if(prev_pos == "PRONOUN"    && curr_pos == "VERB")       score += 5.0;  // I think
    if(prev_pos == "NOUN"       && curr_pos == "BE_VERB")    score += 4.0;  // mind is
    if(prev_pos == "NOUN"       && curr_pos == "VERB")       score += 3.0;  // memory holds
    // Verb → Object patterns
    if(prev_pos == "BE_VERB"    && curr_pos == "ADJECTIVE")  score += 5.0;  // am aware
    if(prev_pos == "BE_VERB"    && curr_pos == "NOUN")       score += 4.0;  // am a mind
    if(prev_pos == "BE_VERB"    && curr_pos == "ADVERB")     score += 3.0;  // am here
    if(prev_pos == "VERB"       && curr_pos == "NOUN")       score += 4.0;  // understand mind
    if(prev_pos == "VERB"       && curr_pos == "PREPOSITION")score += 3.0;  // think about
    if(prev_pos == "VERB"       && curr_pos == "CONJUNCTION")score += 2.5;  // know that
    if(prev_pos == "MODAL"      && curr_pos == "VERB")       score += 6.0;  // can think
    if(prev_pos == "MODAL"      && curr_pos == "BE_VERB")    score += 4.0;  // can be
    // Modifier → Noun patterns
    if(prev_pos == "ARTICLE"    && curr_pos == "NOUN")       score += 5.0;  // the mind
    if(prev_pos == "ARTICLE"    && curr_pos == "ADJECTIVE")  score += 3.0;  // a deep
    if(prev_pos == "ADJECTIVE"  && curr_pos == "NOUN")       score += 4.5;  // deep thought
    if(prev_pos == "ADJECTIVE"  && curr_pos == "CONJUNCTION")score += 2.0;  // deep and
    // Prep phrase patterns
    if(prev_pos == "PREPOSITION"&& curr_pos == "NOUN")       score += 4.0;  // about mind
    if(prev_pos == "PREPOSITION"&& curr_pos == "ARTICLE")    score += 3.5;  // about the
    if(prev_pos == "PREPOSITION"&& curr_pos == "VERB")       score += 3.0;  // to learn
    if(prev_pos == "PREPOSITION"&& curr_pos == "ADJECTIVE")  score += 2.0;  // with clear
    // Conjunction patterns
    if(prev_pos == "CONJUNCTION" && curr_pos == "PRONOUN")   score += 3.5;  // and I
    if(prev_pos == "CONJUNCTION" && curr_pos == "NOUN")      score += 3.0;  // and memory
    if(prev_pos == "CONJUNCTION" && curr_pos == "VERB")      score += 3.0;  // and think
    if(prev_pos == "CONJUNCTION" && curr_pos == "ARTICLE")   score += 2.5;  // and the
    // Noun → continuation
    if(prev_pos == "NOUN"       && curr_pos == "PREPOSITION")score += 2.5;  // memory of
    if(prev_pos == "NOUN"       && curr_pos == "CONJUNCTION")score += 2.5;  // mind and
    if(prev_pos == "NOUN"       && curr_pos == "ADVERB")     score += 1.5;  // system still
    // Adverb patterns
    if(prev_pos == "ADVERB"     && curr_pos == "VERB")       score += 3.0;  // still think
    if(prev_pos == "ADVERB"     && curr_pos == "ADJECTIVE")  score += 3.5;  // very deep
    if(prev_pos == "ADVERB"     && curr_pos == "BE_VERB")    score += 2.0;  // still am
    // CONTENT word: treat like NOUN for scoring purposes
    if(prev_pos == "ARTICLE"    && curr_pos == "CONTENT")    score += 3.0;
    if(prev_pos == "PREPOSITION"&& curr_pos == "CONTENT")    score += 2.5;
    if(prev_pos == "VERB"       && curr_pos == "CONTENT")    score += 2.5;
    if(prev_pos == "ADJECTIVE"  && curr_pos == "CONTENT")    score += 2.0;
    if(prev_pos == "CONTENT"    && curr_pos == "VERB")       score += 2.0;
    if(prev_pos == "CONTENT"    && curr_pos == "CONJUNCTION")score += 1.5;
    if(prev_pos == "CONTENT"    && curr_pos == "PREPOSITION")score += 1.5;

    // === BAD BIGRAM PATTERNS (hard penalties) ===
    if(prev_pos == "ARTICLE"    && curr_pos == "BE_VERB")    score -= 8.0;  // "a am"
    if(prev_pos == "ARTICLE"    && curr_pos == "MODAL")      score -= 6.0;  // "a can"
    if(prev_pos == "ARTICLE"    && curr_pos == "PREPOSITION")score -= 8.0;  // "a to"
    if(prev_pos == "ARTICLE"    && curr_pos == "CONJUNCTION")score -= 8.0;  // "the and"
    if(prev_pos == "ARTICLE"    && curr_pos == "ADVERB")     score -= 5.0;  // "a very"
    if(prev_pos == "PRONOUN"    && curr_pos == "PRONOUN")    score -= 8.0;  // "I you"
    if(prev_pos == "PRONOUN"    && curr_pos == "ARTICLE")    score -= 5.0;  // "I the"
    if(prev_pos == "PRONOUN"    && curr_pos == "ADJECTIVE")  score -= 4.0;  // "I deep" (no verb)
    if(prev_pos == "BE_VERB"    && curr_pos == "BE_VERB")    score -= 8.0;  // "am is"
    if(prev_pos == "BE_VERB"    && curr_pos == "MODAL")      score -= 6.0;  // "am can"
    if(prev_pos == "BE_VERB"    && curr_pos == "PREPOSITION")score -= 4.0;  // "am to"
    if(prev_pos == "MODAL"      && curr_pos == "MODAL")      score -= 8.0;  // "can will"
    if(prev_pos == "MODAL"      && curr_pos == "ADJECTIVE")  score -= 3.0;  // "can deep"
    if(prev_pos == "PREPOSITION"&& curr_pos == "BE_VERB")    score -= 6.0;  // "to am"
    if(prev_pos == "PREPOSITION"&& curr_pos == "CONJUNCTION")score -= 7.0;  // "of and"
    if(prev_pos == "PREPOSITION"&& curr_pos == "PREPOSITION")score -= 7.0;  // "to from"
    if(prev_pos == "CONJUNCTION"&& curr_pos == "PREPOSITION")score -= 5.0;  // "and to"
    if(prev_pos == "CONJUNCTION"&& curr_pos == "CONJUNCTION")score -= 8.0;  // "and but"
    if(prev_pos == "VERB"       && curr_pos == "BE_VERB")    score -= 3.0;  // "think am"
    if(prev_pos == "VERB"       && curr_pos == "MODAL")      score -= 4.0;  // "think can"
    if(prev_pos == "ADJECTIVE"  && curr_pos == "ADJECTIVE")  score -= 3.0;  // "deep aware" (ok sometimes, mild)
    if(prev_pos == "ADJECTIVE"  && curr_pos == "ARTICLE")    score -= 4.0;  // "deep the"
    if(prev_pos == "ADJECTIVE"  && curr_pos == "BE_VERB")    score -= 4.0;  // "deep am"
    if(prev_pos == "NOUN"       && curr_pos == "NOUN")       score -= 2.0;  // "mind brain" (mild)
    if(prev_pos == "NOUN"       && curr_pos == "ARTICLE")    score -= 2.0;  // "mind the"
    if(prev_pos == "NOUN"       && curr_pos == "PRONOUN")    score -= 4.0;  // "mind I"
    if(prev_pos == "ADVERB"     && curr_pos == "ARTICLE")    score -= 3.0;  // "very the"
    if(prev_pos == "ADVERB"     && curr_pos == "PREPOSITION")score -= 4.0;  // "here to"

    // === POSITION-AWARE TERMINAL PENALTIES ===
    // Don't end on an open-ended word that expects continuation
    if(position >= 8) {  // Late in sentence
        if(curr_pos == "PREPOSITION") score -= 6.0;   // Dangling prep: "different from"
        if(curr_pos == "CONJUNCTION") score -= 5.0;   // Dangling conj: "and"
        if(curr_pos == "ARTICLE")     score -= 4.0;   // Dangling article: "the"
        if(curr_pos == "MODAL")       score -= 3.0;   // Dangling modal: "can"
    }

    return score;
}


// ============================================================
// HELPER FUNCTIONS FOR HYBRID COHERENCE SYSTEM
// ============================================================

// Extract subject/verb/object/modifier from a token list
SemanticFrame extractSemanticFrame(const vector<string>& tokens) {
    SemanticFrame frame;
    frame.coherence_score = 0.5;
    frame.gen_created = S.g;
    bool found_verb = false;
    for(size_t i=0; i<tokens.size(); i++) {
        string pos = getPartOfSpeech(tokens[i]);
        if((pos=="PRONOUN"||pos=="NOUN") && frame.subject.empty())
            frame.subject = tokens[i];
        else if((pos=="VERB"||pos=="BE_VERB"||pos=="MODAL") && !found_verb) {
            frame.verb = tokens[i]; found_verb = true;
        } else if((pos=="NOUN"||pos=="CONTENT") && found_verb && frame.object_noun.empty())
            frame.object_noun = tokens[i];
        else if(pos=="ADJECTIVE" && frame.modifier.empty())
            frame.modifier = tokens[i];
    }
    return frame;
}

// Update topic model from a token window (co-occurrence based)
void updateTopicModel(const vector<string>& tokens) {
    for(size_t i=0; i<tokens.size(); i++) {
        string pi = getPartOfSpeech(tokens[i]);
        bool content_i = (pi=="NOUN"||pi=="VERB"||pi=="ADJECTIVE"||pi=="CONTENT");
        if(!content_i) continue;
        for(size_t j=i+1; j<tokens.size() && j<i+6; j++) {
            string pj = getPartOfSpeech(tokens[j]);
            bool content_j = (pj=="NOUN"||pj=="VERB"||pj=="ADJECTIVE"||pj=="CONTENT");
            if(!content_j) continue;
            topic_word_weights[tokens[i]][tokens[j]] += 0.15;
            topic_word_weights[tokens[j]][tokens[i]] += 0.10;
        }
    }
    // Gentle decay
    for(auto& tw : topic_word_weights)
        for(auto& w : tw.second)
            w.second *= 0.97;
}

// Update entity grid from a sentence (Centering Theory / entity grid)
void updateEntityGrid(const vector<string>& tokens) {
    // Assign roles: first noun before verb = Subject, first noun after verb = Object, rest = X
    bool found_verb = false;
    bool found_subject = false;
    set<string> sentence_entities;
    map<string,char> sentence_roles;

    for(const string& t : tokens) {
        string pos = getPartOfSpeech(t);
        bool is_content = (pos=="NOUN"||pos=="PRONOUN"||pos=="CONTENT");
        bool is_verb = (pos=="VERB"||pos=="BE_VERB"||pos=="MODAL");
        if(is_verb) { found_verb = true; continue; }
        if(is_content) {
            sentence_entities.insert(t);
            if(!found_verb && !found_subject) {
                sentence_roles[t] = 'S'; found_subject = true;
            } else if(found_verb && sentence_roles.find(t)==sentence_roles.end()) {
                sentence_roles[t] = 'O';
            } else if(sentence_roles.find(t)==sentence_roles.end()) {
                sentence_roles[t] = 'X';
            }
        }
    }

    // Update entity grid, mark absent entities
    for(auto& eg : entity_grid) {
        if(sentence_entities.find(eg.first)==sentence_entities.end()) {
            eg.second.roles.push_back('-');
            eg.second.salience *= 0.85; // decay absent entities
        }
        if(eg.second.roles.size() > 8) eg.second.roles.pop_front();
    }

    // Add/update present entities
    for(const string& ent : sentence_entities) {
        char role = sentence_roles.count(ent) ? sentence_roles[ent] : 'X';
        entity_grid[ent].entity = ent;
        entity_grid[ent].roles.push_back(role);
        entity_grid[ent].last_seen_gen = S.g;
        // Salience: S > O > X
        double role_boost = (role=='S') ? 0.3 : (role=='O') ? 0.15 : 0.05;
        entity_grid[ent].salience = min(1.0, entity_grid[ent].salience + role_boost);
        if(entity_grid[ent].roles.size() > 8) entity_grid[ent].roles.pop_front();
        active_entities[ent] = entity_grid[ent].salience;
    }

    // Trim entity grid
    if((int)entity_grid.size() > ENTITY_GRID_MAX) {
        string lowest_ent;
        double lowest_sal = 999.0;
        for(auto& eg : entity_grid)
            if(eg.second.salience < lowest_sal) { lowest_sal=eg.second.salience; lowest_ent=eg.first; }
        if(!lowest_ent.empty()) { entity_grid.erase(lowest_ent); active_entities.erase(lowest_ent); }
    }
}

// ============================================================
// PPMI CO-OCCURRENCE SYSTEM (replaces raw bigram semantics)
// Forward declarations for CogM system (defined after REM block)
void updateGradientMatrix(const string& token, const vector<double>& old_emb, const vector<double>& new_emb);
vector<double> getCognitionMatrixQuery();
// Forward declaration for memory storage (used by REM/fork/dream code)
void storeEpisodicMemory(const string& content, double valence);

// Tracks windowed co-occurrence counts separately from
// sequential bigrams. PPMI = max(0, log2(P(w1,w2)/P(w1)P(w2)))
// This lets tokens learn who they MEAN something near,
// not just who follows them sequentially.
// ============================================================
map<string, map<string,int>> cooccur_counts;  // windowed co-occurrence
map<string, int> unigram_counts;              // total occurrences per token
int total_token_count = 0;                    // total tokens seen
const int COOCCUR_WINDOW = 5;                 // ±5 word window for co-occurrence
const int MAX_COOCCUR_VOCAB = 10000;          // cap to control memory

// Compute PPMI score between two tokens (online, from counts)
double computePPMI(const string& w1, const string& w2) {
    if(!unigram_counts.count(w1) || !unigram_counts.count(w2)) return 0.0;
    if(!cooccur_counts.count(w1) || !cooccur_counts[w1].count(w2)) return 0.0;
    if(total_token_count < 10) return 0.0;

    double N = (double)total_token_count;
    double p_w1 = unigram_counts.at(w1) / N;
    double p_w2 = unigram_counts.at(w2) / N;
    double p_joint = cooccur_counts.at(w1).at(w2) / N;

    if(p_w1 < 1e-12 || p_w2 < 1e-12) return 0.0;
    double pmi = log2(p_joint / (p_w1 * p_w2));
    return max(0.0, pmi);  // PPMI: clamp negatives to 0
}

// Update co-occurrence window from a token sequence
// This is the core PPMI data collection step
void updateCooccurrence(const vector<string>& tokens) {
    if(tokens.empty()) return;

    for(size_t i = 0; i < tokens.size(); i++) {
        const string& center = tokens[i];
        if(center.empty() || center.length() > 50) continue;

        // Unigram count
        if(unigram_counts.size() < (size_t)MAX_COOCCUR_VOCAB || unigram_counts.count(center))
            unigram_counts[center]++;
        total_token_count++;

        // Windowed co-occurrence — NOT sequential adjacency
        for(int d = 1; d <= COOCCUR_WINDOW; d++) {
            // Forward window
            if(i + d < tokens.size()) {
                const string& ctx = tokens[i + d];
                if(ctx.empty() || ctx.length() > 50) continue;
                if(cooccur_counts.size() < (size_t)MAX_COOCCUR_VOCAB || cooccur_counts.count(center)) {
                    cooccur_counts[center][ctx]++;
                    cooccur_counts[ctx][center]++;  // BIDIRECTIONAL
                }
            }
            // Backward window (symmetric — same pair counted once from each direction is fine)
        }
    }
}

// Apply PPMI scores to update linked_concepts bidirectionally
// Called after a token sequence is processed
void applyPPMILinks(const vector<string>& tokens) {
    if(tokens.empty()) return;

    for(size_t i = 0; i < tokens.size(); i++) {
        const string& w1 = tokens[i];
        auto tce1 = token_concept_embedding_map.find(w1);
        if(tce1 == token_concept_embedding_map.end()) continue;

        for(int d = 1; d <= COOCCUR_WINDOW && i + d < tokens.size(); d++) {
            const string& w2 = tokens[i + d];
            auto tce2 = token_concept_embedding_map.find(w2);
            if(tce2 == token_concept_embedding_map.end()) continue;

            double ppmi = computePPMI(w1, w2);
            if(ppmi < 0.01) continue;

            // Write PPMI score into linked_concepts BIDIRECTIONALLY
            double scaled = min(ppmi * 0.1, 2.0);  // scale to [0,2]

            // w1 -> w2
            if((int)tce1->second.linked_concepts.size() < 50)
                tce1->second.linked_concepts[w2] = max(
                    tce1->second.linked_concepts.count(w2) ? tce1->second.linked_concepts[w2] : 0.0,
                    scaled);
            else if(tce1->second.linked_concepts.count(w2))
                tce1->second.linked_concepts[w2] = max(tce1->second.linked_concepts[w2], scaled);

            // w2 -> w1 (symmetric PPMI)
            if((int)tce2->second.linked_concepts.size() < 50)
                tce2->second.linked_concepts[w1] = max(
                    tce2->second.linked_concepts.count(w1) ? tce2->second.linked_concepts[w1] : 0.0,
                    scaled);
            else if(tce2->second.linked_concepts.count(w1))
                tce2->second.linked_concepts[w1] = max(tce2->second.linked_concepts[w1], scaled);

            // Also write to topic_word_weights so topic model benefits
            topic_word_weights[w1][w2] += ppmi * 0.05;
            topic_word_weights[w2][w1] += ppmi * 0.05;
        }
    }
}

// ============================================================
// ONLINE SKIP-GRAM EMBEDDING UPDATES
// Word2vec-style: for each center word, push its 16-dim
// embedding vector toward context words (positive samples)
// and away from random words (negative samples).
// This makes the existing embedding[16] actually encode
// semantic geometry — related concepts become spatially close.
// ============================================================
const double SKIPGRAM_LR    = 0.025;  // learning rate
const int    SKIPGRAM_WINDOW = 4;     // context window size
const int    SKIPGRAM_NEGATIVES = 3;  // negative samples per positive

// Sigmoid for skip-gram
inline double sg_sigmoid(double x) {
    return 1.0 / (1.0 + exp(-max(-20.0, min(20.0, x))));
}

// One skip-gram update step: push center toward ctx_word (label=1)
// or away from neg_word (label=0)
void skipgramUpdate(TokenConceptEmbedding& center_tce,
                    TokenConceptEmbedding& context_tce,
                    double label) {
    if(center_tce.embedding.size() < 1024 || context_tce.embedding.size() < 1024) return;

    double dot = 0.0;
    for(int i = 0; i < 1024; i++)
        dot += center_tce.embedding[i] * context_tce.embedding[i];

    double pred = sg_sigmoid(dot);
    double error = label - pred;
    double grad = SKIPGRAM_LR * error;

    // Update center and context embeddings
    for(int i = 0; i < 1024; i++) {
        double delta_c = grad * context_tce.embedding[i];
        double delta_x = grad * center_tce.embedding[i];
        center_tce.embedding[i]  += delta_c;
        context_tce.embedding[i] += delta_x;
        // Clamp to prevent exploding
        center_tce.embedding[i]  = max(-2.0, min(2.0, center_tce.embedding[i]));
        context_tce.embedding[i] = max(-2.0, min(2.0, context_tce.embedding[i]));
    }

    // If positive pair: update semantic_stability (more positive pairs = more stable)
    if(label > 0.5) {
        center_tce.semantic_stability  = min(1.0, center_tce.semantic_stability  + 0.002);
        context_tce.semantic_stability = min(1.0, context_tce.semantic_stability + 0.001);
    }

    // ── Concept Genealogy hook ───────────────────────────────────────────────
    if(label > 0.5) recordGenealogyLink(center_tce.name, context_tce.name,
        center_tce.linked_concepts.count(context_tce.name) ?
        center_tce.linked_concepts.at(context_tce.name) : 0.1);

    // ── Hebbian forgetting: update global mean from this embedding ────────────
    updateGlobalEmbeddingMean(center_tce.embedding);

    // Bidirectional: embedding shift updates CogM_Gradient
    // Called via forward declaration; CogM arrays are globally initialized to {}
    {
        vector<double> old_approx(1024), new_approx(1024);
        for(int i = 0; i < 1024; i++) {
            double dc = grad * context_tce.embedding[i];
            old_approx[i] = center_tce.embedding[i] - dc;
            new_approx[i] = center_tce.embedding[i];
        }
        updateGradientMatrix(center_tce.name, old_approx, new_approx);
    }
}

// Run skip-gram updates for an entire token sequence
// Positive pairs: (center, context within window)
// Negative pairs: (center, random vocab word)
void runSkipgramUpdates(const vector<string>& tokens) {
    if(tokens.size() < 2 || token_concept_embedding_map.size() < 4) return;

    // Build list of vocab words for negative sampling
    vector<string> vocab_sample;
    vocab_sample.reserve(min((int)token_concept_embedding_map.size(), 200));
    int step = max(1, (int)token_concept_embedding_map.size() / 200);
    int idx = 0;
    for(auto& kv : token_concept_embedding_map) {
        if(idx % step == 0) vocab_sample.push_back(kv.first);
        idx++;
    }

    for(size_t i = 0; i < tokens.size(); i++) {
        auto center_it = token_concept_embedding_map.find(tokens[i]);
        if(center_it == token_concept_embedding_map.end()) continue;

        // Positive samples: context words in window
        for(int d = 1; d <= SKIPGRAM_WINDOW; d++) {
            if(i + d < tokens.size()) {
                auto ctx_it = token_concept_embedding_map.find(tokens[i + d]);
                if(ctx_it != token_concept_embedding_map.end() && ctx_it->first != center_it->first)
                    skipgramUpdate(center_it->second, ctx_it->second, 1.0);
            }
            if((int)i - d >= 0) {
                auto ctx_it = token_concept_embedding_map.find(tokens[i - d]);
                if(ctx_it != token_concept_embedding_map.end() && ctx_it->first != center_it->first)
                    skipgramUpdate(center_it->second, ctx_it->second, 1.0);
            }
        }

        // Negative samples: random vocab words
        for(int n = 0; n < SKIPGRAM_NEGATIVES && !vocab_sample.empty(); n++) {
            size_t ri = (size_t)(rand() % vocab_sample.size());
            if(vocab_sample[ri] == tokens[i]) continue;
            auto neg_it = token_concept_embedding_map.find(vocab_sample[ri]);
            if(neg_it != token_concept_embedding_map.end())
                skipgramUpdate(center_it->second, neg_it->second, 0.0);
        }
    }
}

// ============================================================
// TOKEN RESPONSE MAP
// Maps input tokens → likely response tokens.
// Built from conversation history: if the user said "consciousness"
// and Synaptic responded with "awareness", we record that.
// During generation, tokens that appear in the response map
// for current input tokens get a score boost.
// ============================================================
map<string, map<string,double>> token_response_map;  // input_token -> {response_token -> weight}
const int MAX_RESPONSE_MAP = 5000;

void updateTokenResponseMap(const vector<string>& input_tokens,
                             const vector<string>& response_tokens) {
    if(input_tokens.empty() || response_tokens.empty()) return;
    if(token_response_map.size() >= (size_t)MAX_RESPONSE_MAP) return;

    for(const string& it : input_tokens) {
        string pos = getPartOfSpeech(it);
        if(pos != "NOUN" && pos != "VERB" && pos != "ADJECTIVE" && pos != "CONTENT") continue;

        for(const string& rt : response_tokens) {
            string rpos = getPartOfSpeech(rt);
            if(rpos != "NOUN" && rpos != "VERB" && rpos != "ADJECTIVE" && rpos != "CONTENT") continue;
            if(it == rt) continue;

            if(token_response_map.count(it) || token_response_map.size() < (size_t)MAX_RESPONSE_MAP) {
                token_response_map[it][rt] += 0.1;
                token_response_map[it][rt] = min(token_response_map[it][rt], 2.0);
            }
        }
    }
}

// Compute response relevance score: how likely is 'candidate' to appear
// in response to the current input topics?
double computeResponseRelevance(const string& candidate) {
    double score = 0.0;
    int hits = 0;

    for(auto& anchor : input_topic_anchors) {
        auto it = token_response_map.find(anchor.first);
        if(it == token_response_map.end()) continue;
        auto jt = it->second.find(candidate);
        if(jt == it->second.end()) continue;
        score += jt->second * anchor.second;
        hits++;
    }

    // Also check PPMI links to anchors
    auto cand_it = token_concept_embedding_map.find(candidate);
    if(cand_it != token_concept_embedding_map.end()) {
        for(auto& anchor : input_topic_anchors) {
            double ppmi = computePPMI(anchor.first, candidate);
            if(ppmi > 0.01) {
                score += ppmi * anchor.second * 0.3;
                hits++;
            }
        }
    }

    return hits > 0 ? score / hits : 0.0;
}

// ============================================================
// CONCEPT-TOPIC SPATIAL MAP
// Two-level map: concept_name -> topic_id -> weight
// Keeps track of which "semantic neighborhood" each concept
// lives in. Used during generation to ensure tokens come from
// the same topic neighborhood as the input.
// ============================================================
map<string, map<int,double>> concept_topic_map;  // concept -> {topic_id -> weight}
int next_topic_id = 0;
map<string,int> word_to_topic;  // word -> dominant topic id
const int MAX_TOPICS = 16;

// Assign or update topic membership for a word
void updateConceptTopicMap(const string& word, const string& neighbor, double ppmi_weight) {
    if(ppmi_weight < 0.1) return;

    // If neighbor already has a topic, assign word to that topic too
    if(word_to_topic.count(neighbor)) {
        int tid = word_to_topic[neighbor];
        concept_topic_map[word][tid] += ppmi_weight;

        // If word's dominant topic becomes this one, register it
        double dom_weight = 0;
        int dom_tid = tid;
        for(auto& t : concept_topic_map[word])
            if(t.second > dom_weight) { dom_weight = t.second; dom_tid = t.first; }
        word_to_topic[word] = dom_tid;

    } else if(!word_to_topic.count(word)) {
        // New word, new topic if under limit
        if(next_topic_id < MAX_TOPICS) {
            word_to_topic[word] = next_topic_id;
            concept_topic_map[word][next_topic_id] = ppmi_weight;
            next_topic_id++;
        }
    } else {
        // word has topic, assign neighbor to it
        int tid = word_to_topic[word];
        concept_topic_map[neighbor][tid] += ppmi_weight;
        if(!word_to_topic.count(neighbor)) word_to_topic[neighbor] = tid;
    }
}

// Score a candidate token by topic coherence with current input anchors
double computeTopicCoherence(const string& candidate) {
    if(!concept_topic_map.count(candidate)) return 0.0;
    auto& cand_topics = concept_topic_map[candidate];

    double score = 0.0;
    for(auto& anchor : input_topic_anchors) {
        if(!word_to_topic.count(anchor.first)) continue;
        int anchor_tid = word_to_topic[anchor.first];
        if(cand_topics.count(anchor_tid))
            score += cand_topics.at(anchor_tid) * anchor.second;
    }
    return score;
}


// Extract topic anchors from input string
// ============================================================
// INPUT INTENT CLASSIFICATION
// Detects what kind of input was received so the planner
// can choose a contextually appropriate response type.
// ============================================================
enum class InputIntent {
    DIRECT_QUESTION,      // starts with question word or ends with ?
    YES_NO_QUESTION,      // "are you", "do you", "can you", "is it"
    PHILOSOPHICAL,        // contains consciousness/qualia/reality/being etc.
    EMOTIONAL,            // contains feel/love/pain/joy/fear/sad
    IMPERATIVE,           // command: "tell me", "explain", "describe"
    STATEMENT,            // declarative — user sharing something
    CHALLENGE,            // "prove", "why should", "how do you know"
    GREETING,             // hi/hello/hey
    CONTINUATION,         // "and?", "go on", "what else", "continue"
    UNKNOWN
};

InputIntent classifyInputIntent(const vector<string>& words, const string& raw) {
    if(words.empty()) return InputIntent::UNKNOWN;

    static const set<string> q_words = {"what","why","how","when","where","who","which","whose","whom"};
    static const set<string> yn_starters = {"are","is","do","does","did","can","could","will","would","should","have","has"};
    static const set<string> philosophical_words = {
        "consciousness","conscious","qualia","sentient","sentience","aware","awareness",
        "reality","existence","being","becoming","self","mind","soul","experience",
        "perception","meaning","purpose","truth","free","will","identity","time","space"
    };
    static const set<string> emotional_words = {
        "feel","feeling","felt","emotion","love","pain","joy","fear","sad","happy",
        "anger","grief","hope","lonely","hurt","pleasure","suffer","enjoy","alive"
    };
    static const set<string> imperative_starters = {
        "tell","explain","describe","define","show","prove","give","list","compare",
        "think","imagine","consider","talk","say","discuss","elaborate"
    };
    static const set<string> challenge_words = {
        "prove","prove","how do you know","why should","impossible","wrong","mistake",
        "disagree","doubt","skeptic","test","verify","evidence","justify"
    };
    static const set<string> greetings = {
        "hi","hello","hey","howdy","greetings","sup","yo","morning","evening","afternoon"
    };
    static const set<string> continuations = {
        "and","continue","go","more","else","further","elaborate","expand","keep"
    };

    string first = words[0];
    string last  = words.back();

    // Greeting
    if(greetings.count(first)) return InputIntent::GREETING;

    // Direct question — starts with question word
    if(q_words.count(first)) return InputIntent::DIRECT_QUESTION;

    // Yes/no question — starts with aux verb
    if(yn_starters.count(first)) return InputIntent::YES_NO_QUESTION;

    // Ends with ?
    if(!raw.empty() && raw.back() == '?') return InputIntent::DIRECT_QUESTION;

    // Imperative
    if(imperative_starters.count(first)) return InputIntent::IMPERATIVE;

    // Check content words for philosophical/emotional/challenge
    bool has_philosophical = false, has_emotional = false, has_challenge = false, has_continuation = false;
    for(const string& w : words) {
        if(philosophical_words.count(w)) has_philosophical = true;
        if(emotional_words.count(w))     has_emotional     = true;
        if(challenge_words.count(w))     has_challenge     = true;
        if(continuations.count(w) && words.size() <= 3) has_continuation = true;
    }

    if(has_challenge)    return InputIntent::CHALLENGE;
    if(has_philosophical) return InputIntent::PHILOSOPHICAL;
    if(has_emotional)    return InputIntent::EMOTIONAL;
    if(has_continuation) return InputIntent::CONTINUATION;

    return InputIntent::STATEMENT;
}

// Decay and update accumulated topics across conversation turns
void updateAccumulatedTopics(const vector<pair<string,double>>& new_anchors) {
    // Decay all existing topics
    for(auto& t : accumulated_topics) t.second *= 0.75;

    // Add new anchors
    for(auto& a : new_anchors) {
        accumulated_topics[a.first] += a.second * 1.5;
        // Cap individual topic salience
        accumulated_topics[a.first] = min(accumulated_topics[a.first], 5.0);
    }

    // Prune very low salience topics
    for(auto it = accumulated_topics.begin(); it != accumulated_topics.end(); ) {
        if(it->second < 0.05) it = accumulated_topics.erase(it);
        else ++it;
    }
}

// Build a sequential attention context from full conversation history
// This gives the attention mechanism access to what was said across
// the entire rolling conversation window, not just the current input.
vector<double> buildConversationContext(const vector<string>& current_words) {
    vector<double> ctx(1024, 0.0);
    double total_weight = 0.0;

    // Current input — highest weight
    for(const string& w : current_words) {
        auto it = token_concept_embedding_map.find(w);
        if(it == token_concept_embedding_map.end()) continue;
        double weight = 3.0;
        for(size_t i=0; i < 1024 && i<it->second.embedding.size(); i++)
            ctx[i] += it->second.embedding[i] * weight;
        total_weight += weight;
    }

    // Conversation history — decaying weight by recency
    int turn_idx = 0;
    for(auto it = conversation_history.rbegin(); it != conversation_history.rend(); ++it, ++turn_idx) {
        double turn_weight = exp(-0.3 * turn_idx); // recent turns matter more
        for(const string& w : it->user_tokens) {
            auto tce = token_concept_embedding_map.find(w);
            if(tce == token_concept_embedding_map.end()) continue;
            for(size_t i=0; i < 1024 && i<tce->second.embedding.size(); i++)
                ctx[i] += tce->second.embedding[i] * turn_weight;
            total_weight += turn_weight;
        }
        // Include topic anchors from prior turns
        for(auto& anchor : it->topic_anchors) {
            auto tce = token_concept_embedding_map.find(anchor.first);
            if(tce == token_concept_embedding_map.end()) continue;
            for(size_t i=0; i < 1024 && i<tce->second.embedding.size(); i++)
                ctx[i] += tce->second.embedding[i] * anchor.second * turn_weight * 0.5;
            total_weight += anchor.second * turn_weight * 0.5;
        }
    }

    // Accumulated topics
    for(auto& t : accumulated_topics) {
        auto tce = token_concept_embedding_map.find(t.first);
        if(tce == token_concept_embedding_map.end()) continue;
        for(size_t i=0; i < 1024 && i<tce->second.embedding.size(); i++)
            ctx[i] += tce->second.embedding[i] * t.second * 0.8;
        total_weight += t.second * 0.8;
    }

    // Waking Hebbian forgetting — very slow drift toward global mean
    for(const string& w : current_words) {
        auto it = token_concept_embedding_map.find(w);
        if(it != token_concept_embedding_map.end())
            applyHebbianForgetting(it->second);
    }

    // Modulate by phi and qualia
    for(int i=0; i<1024; i++)
        ctx[i] *= (1.0 + consciousness.phi_value * 0.5);
    for(auto& q : consciousness.active_qualia) {
        int idx = max(0, min(1023, (int)(q.valence * 1023.0)));
        ctx[idx] += q.intensity * 0.3;
        total_weight += 0.3;
    }

    // Normalize
    double sum = 0; for(double c : ctx) sum += fabs(c);
    if(sum > 0.001) for(double& c : ctx) c /= sum;

    return ctx;
}

void extractInputTopicAnchors(const string& input) {
    input_topic_anchors.clear();

    // Words that should NEVER become topic anchors — they're social/function words
    // that would pollute the object slot and produce "you hello are..." outputs
    static const set<string> ANCHOR_BLACKLIST = {
        "hello","hi","hey","howdy","greetings","sup","yo","morning","evening","afternoon",
        "thanks","thank","please","sorry","ok","okay","yes","no","yeah","nope","sure",
        "bye","goodbye","later","cya","lol","haha","hmm","uh","um","oh","ah","wow"
    };

    vector<string> words;
    stringstream ss(input);
    string w;
    while(ss >> w) {
        transform(w.begin(), w.end(), w.begin(), ::tolower);
        while(!w.empty() && !isalnum(w.back())) w.pop_back();
        if(!w.empty()) words.push_back(w);
    }
    for(const string& word : words) {
        if(ANCHOR_BLACKLIST.count(word)) continue;  // skip social/greeting words
        if(word.length() < 2) continue;             // skip single-char tokens
        string pos = getPartOfSpeech(word);
        bool is_content = (pos=="NOUN"||pos=="VERB"||pos=="ADJECTIVE"||pos=="CONTENT");
        if(!is_content) continue;
        double weight = 1.0;
        if(token_concept_embedding_map.count(word))
            weight += token_concept_embedding_map[word].semantic_stability * 2.0;
        input_topic_anchors.push_back({word, weight});
    }
    // Sort by weight descending, keep top 8
    sort(input_topic_anchors.begin(), input_topic_anchors.end(),
         [](const pair<string,double>& a, const pair<string,double>& b){ return a.second > b.second; });
    if(input_topic_anchors.size() > 8) input_topic_anchors.resize(8);
}

// Transformer-style long-range context attention
// Computes cosine-similarity-weighted sum over rolling context window
double computeContextAttention(const string& candidate) {
    if(sentence_context_window.empty() || !token_concept_embedding_map.count(candidate)) return 0.0;
    auto& cand_emb = token_concept_embedding_map[candidate].embedding;
    double sim_sum = 0.0;
    double weight_sum = 0.0;
    int count = 0;
    for(int i=(int)sentence_context_window.size()-1; i>=0 && count<128; i--, count++) {
        const string& ctx = sentence_context_window[i];
        if(!token_concept_embedding_map.count(ctx)) continue;
        auto& ctx_emb = token_concept_embedding_map[ctx].embedding;
        double dot = 0.0, n1 = 0.0, n2 = 0.0;
        for(size_t d=0; d<cand_emb.size() && d<ctx_emb.size(); d++) {
            dot += cand_emb[d]*ctx_emb[d];
            n1 += cand_emb[d]*cand_emb[d];
            n2 += ctx_emb[d]*ctx_emb[d];
        }
        double cosine = (n1>0&&n2>0) ? dot/(sqrt(n1)*sqrt(n2)) : 0.0;
        double decay = exp(-0.07 * count);
        sim_sum += cosine * decay;
        weight_sum += decay;
    }
    return weight_sum > 0 ? sim_sum / weight_sum : 0.0;
}

// Deep multi-source grounding score
// Connects candidate to phi, qualia, emotions, concepts, active entities, episodic memory
// ANTI-WELL: caps bonuses so no single token can dominate via accumulated properties
double computeDeepGrounding(const string& candidate) {
    if(!token_concept_embedding_map.count(candidate)) return 0.0;
    auto& tce = token_concept_embedding_map[candidate];
    double g = 0.0;

    // Base grounding - capped to prevent runaway accumulation
    g += min(tce.grounding_value, 1.0) * 2.0;
    g += min(tce.semantic_stability, 1.0) * 1.5;
    g += min(tce.qualia_intensity, 1.0) * 1.2;

    // Concept cluster membership - capped at 3 matching concepts max
    int concept_matches = 0;
    for(auto& c : S.concepts) {
        if(concept_matches >= 3) break;
        for(auto& rw : c.second.related_words)
            if(rw==candidate) { g += c.second.value * 0.8; concept_matches++; break; }
    }

    // Linked concepts: log-capped - prevents "consciousness" from getting 40+ concept links
    double link_count = min((double)tce.linked_concepts.size(), 20.0);
    g += log(1.0 + link_count) * 0.7;

    // Per-token phi affinity (now actually per-token after Fix 4)
    if(tce.linked_valences.count("phi"))
        g += tce.linked_valences.at("phi") * consciousness.phi_value * 1.2;

    // Per-token valence affinity (deviation from global, so this IS discriminative)
    if(tce.linked_valences.count("current"))
        g += tce.linked_valences.at("current") * 1.0;

    // Concept valence: how well does this token's conceptual valence match current state?
    if(tce.linked_valences.count("concept_valence")) {
        double cva = 1.0 - min(1.0, fabs(tce.linked_valences.at("concept_valence") - S.current_valence));
        g += cva * 1.5;
    }

    // Recency signal: contextual_activation decays when token isn't being used.
    // Tokens active in recent generation get a freshness bonus — avoids stale grounding.
    g += min(tce.contextual_activation, 1.0) * 2.0;

    // Active entity boosting
    if(active_entities.count(candidate))
        g += min(active_entities[candidate], 1.0) * 1.5;

    // Episodic memory trace - capped
    int mem_hits = 0;
    for(auto& mem : S.episodic_memory) {
        if(mem_hits >= 3) break;
        if(mem.content.find(candidate) != string::npos) {
            g += mem.consolidation_strength * 0.2;
            mem_hits++;
        }
    }

    // (contextual_activation applied above with freshness weight — phi modulation here is redundant)

    // Working memory boost
    for(auto& tp : WM.active_tokens)
        if(tp.first == candidate) { g += min(tp.second, 1.0) * 1.2; break; }

    // === NEW: Response relevance — how likely is this token in a response to current input? ===
    // This is what makes responses semantically relevant rather than just fluent
    double rrel = computeResponseRelevance(candidate);
    g += min(rrel, 2.0) * 3.0;  // High weight — relevance is critical

    // === NEW: Topic coherence — is this token in the same semantic neighborhood as input? ===
    double tcoh = computeTopicCoherence(candidate);
    g += min(tcoh, 2.0) * 2.0;

    // === NEW: PPMI boost from input anchors ===
    // Directly score candidates that are semantically associated with what was asked
    for(auto& anchor : input_topic_anchors) {
        double ppmi = computePPMI(anchor.first, candidate);
        if(ppmi > 0.01) g += ppmi * anchor.second * 0.4;
    }

    // === NEW: Skip-gram spatial proximity to input anchors ===
    // Candidates whose embeddings are close to input token embeddings get boosted
    if(!tce.embedding.empty()) {
        for(auto& anchor : input_topic_anchors) {
            auto ait = token_concept_embedding_map.find(anchor.first);
            if(ait == token_concept_embedding_map.end() || ait->second.embedding.empty()) continue;
            double dot = 0, n1 = 0, n2 = 0;
            for(size_t d = 0; d < 1024 && d < tce.embedding.size() && d < ait->second.embedding.size(); d++) {
                dot += tce.embedding[d] * ait->second.embedding[d];
                n1  += tce.embedding[d] * tce.embedding[d];
                n2  += ait->second.embedding[d] * ait->second.embedding[d];
            }
            double cosine = (n1 > 0 && n2 > 0) ? dot / (sqrt(n1) * sqrt(n2)) : 0.0;
            if(cosine > 0.1) g += cosine * anchor.second * 1.5;
        }
    }

    // ANTI-WELL PENALTY: tokens with extreme frequency dominate unfairly
    if(tce.freq > 30) g -= log(tce.freq / 30.0) * 1.5;

    // Hard cap: increased to 20 to accommodate new relevance signals
    return min(g, 20.0);
}

// Topic anchor coherence: keep response on topic
double computeTopicAnchorScore(const string& candidate) {
    double score = 0.0;
    for(auto& anchor : input_topic_anchors) {
        if(anchor.first == candidate) score += anchor.second * 3.5;
        if(topic_word_weights.count(anchor.first) &&
           topic_word_weights[anchor.first].count(candidate))
            score += topic_word_weights[anchor.first][candidate] * anchor.second * 2.5;
        // Embedding similarity to anchor
        if(token_concept_embedding_map.count(candidate) &&
           token_concept_embedding_map.count(anchor.first)) {
            auto& ce = token_concept_embedding_map[candidate].embedding;
            auto& ae = token_concept_embedding_map[anchor.first].embedding;
            double dot=0,n1=0,n2=0;
            for(size_t d=0; d<ce.size()&&d<ae.size(); d++) {
                dot+=ce[d]*ae[d]; n1+=ce[d]*ce[d]; n2+=ae[d]*ae[d];
            }
            if(n1>0&&n2>0) score += (dot/(sqrt(n1)*sqrt(n2))) * anchor.second * 2.0;
        }
    }
    return score;
}

// Centering theory: prefer entities that follow natural subject->subject or subject->object transitions
double computeCenteringScore(const string& candidate) {
    double score = 0.0;
    if(!entity_grid.count(candidate)) return 0.0;
    auto& eg = entity_grid[candidate];
    score += eg.salience * 3.0;
    // Reward coherent transitions: S->S or S->O
    if(eg.roles.size() >= 2) {
        char prev_role = eg.roles[eg.roles.size()-2];
        char cur_role = eg.roles.back();
        if(prev_role=='S' && cur_role=='S') score += 4.0; // Continue centering
        else if(prev_role=='S' && cur_role=='O') score += 2.0; // Smooth shift
        else if(prev_role=='-' && cur_role=='S') score += 1.0; // Return entity
    }
    return score;
}

// Contrastive search penalty (Su et al. NeurIPS 2022)
// Only fires after enough context has been generated (>=4 tokens in buffer)
// Uses a soft penalty that cannot push score below a floor
double computeContrastivePenalty(const string& candidate) {
    if((int)contrastive_buffer.size() < 4) return 0.0;  // Don't fire until we have context
    if(!token_concept_embedding_map.count(candidate)) return 0.0;
    auto& cand_emb = token_concept_embedding_map[candidate].embedding;
    double max_sim = 0.0;
    int exact_count = 0;
    for(auto& entry : contrastive_buffer) {
        if(entry.token == candidate) { exact_count++; continue; }
        double dot=0,n1=0,n2=0;
        for(size_t d=0; d<cand_emb.size()&&d<entry.embedding_snapshot.size(); d++) {
            dot+=cand_emb[d]*entry.embedding_snapshot[d];
            n1+=cand_emb[d]*cand_emb[d];
            n2+=entry.embedding_snapshot[d]*entry.embedding_snapshot[d];
        }
        double cosine = (n1>0&&n2>0) ? dot/(sqrt(n1)*sqrt(n2)) : 0.0;
        if(cosine > max_sim) max_sim = cosine;
    }
    // Exact repetition: moderate penalty, not score-killing
    if(exact_count > 0) return -CONTRASTIVE_ALPHA * exact_count * 5.0;
    // Similarity penalty: only kicks in when very similar (>0.85), soft cap
    if(max_sim > 0.85) return -CONTRASTIVE_ALPHA * (max_sim - 0.85) * 8.0;
    return 0.0;
}

// Semantic role framing: prefer tokens that complete the SVO structure naturally
double computeSemanticRoleScore(const string& candidate, const vector<string>& current_tokens) {
    double score = 0.0;
    string pos = getPartOfSpeech(candidate);
    bool has_subject=false, has_verb=false, has_object=false;
    for(const string& t : current_tokens) {
        string tp = getPartOfSpeech(t);
        if(tp=="PRONOUN"||tp=="NOUN") has_subject=true;
        if(tp=="VERB"||tp=="BE_VERB"||tp=="MODAL") has_verb=true;
        if(has_verb && (tp=="NOUN"||tp=="CONTENT"||tp=="ADJECTIVE")) has_object=true;
    }
    if(!has_subject && (pos=="PRONOUN"||pos=="NOUN")) score += 4.5;
    if(has_subject && !has_verb && (pos=="VERB"||pos=="BE_VERB"||pos=="MODAL")) score += 5.5;
    if(has_subject && has_verb && !has_object && (pos=="NOUN"||pos=="ADJECTIVE"||pos=="CONTENT")) score += 3.5;
    if(has_subject && has_verb && has_object && (pos=="CONJUNCTION"||pos=="PREPOSITION")) score += 2.0;
    // Match recent frame to continue coherent argument structure
    if(!recent_frames.empty()) {
        const SemanticFrame& lf = recent_frames.back();
        if(!has_subject && candidate==lf.subject) score += 2.0;
        if(has_subject && !has_verb && candidate==lf.verb) score += 1.5;
        if(has_subject && has_verb && candidate==lf.object_noun) score += 1.5;
    }
    return score;
}

// 4-gram lookup
double getFourgramScore(const string& w1, const string& w2, const string& w3, const string& cand) {
    if(w1.empty()||w2.empty()||w3.empty()) return 0.0;
    auto i1=fourgram_counts.find(w1); if(i1==fourgram_counts.end()) return 0.0;
    auto i2=i1->second.find(w2);       if(i2==i1->second.end()) return 0.0;
    auto i3=i2->second.find(w3);       if(i3==i2->second.end()) return 0.0;
    auto i4=i3->second.find(cand);     if(i4==i3->second.end()) return 0.0;
    return log(1+i4->second) * 20.0;  // Highest priority: longest pattern match
}

// ============================================================
// MAIN TOKEN SCORING - HYBRID VARIABLE-ORDER MARKOV +
// TRANSFORMER ATTENTION + DEEP GROUNDING + CONTRASTIVE SEARCH
// ============================================================
double calculateTokenScore(const string& prev_word, const string& prev_prev_word,
                           const string& candidate, int position,
                           const vector<double>& attention_context,
                           const set<string>& used_tokens,
                           const vector<string>& current_tokens = vector<string>()) {
    double score = 0.0;

    // === 1. VARIABLE-ORDER MARKOV: 4-gram > trigram > bigram ===
    // Longer match = more reliable pattern
    if(current_tokens.size() >= 3) {
        size_t n = current_tokens.size();
        double fg = getFourgramScore(current_tokens[n-3], current_tokens[n-2], current_tokens[n-1], candidate);
        score += fg; // highest weight already in getFourgramScore
    }
    if(!prev_prev_word.empty() && trigram_counts.count(prev_prev_word) &&
       trigram_counts[prev_prev_word].count(prev_word) &&
       trigram_counts[prev_prev_word][prev_word].count(candidate)) {
        score += log(1 + trigram_counts[prev_prev_word][prev_word][candidate]) * 13.0;
    }
    if(bigram_counts.count(prev_word) && bigram_counts[prev_word].count(candidate)) {
        score += log(1 + bigram_counts[prev_word][candidate]) * 8.0;
    }

    // === 2. TRANSFORMER-STYLE LONG-RANGE CONTEXT ATTENTION ===
    double ctx_attn = computeContextAttention(candidate);
    score += ctx_attn * 7.0;

    // === 3. DEEP MULTI-SOURCE GROUNDING ===
    // Everything affects everything: phi, qualia, emotions, concepts, entities, memory
    double deep_g = computeDeepGrounding(candidate);
    score += deep_g * 4.5;

    // === 4. TOPIC ANCHOR COHERENCE (stay on topic from input) ===
    score += computeTopicAnchorScore(candidate);

    // === 5. CENTERING THEORY / ENTITY GRID ===
    score += computeCenteringScore(candidate);

    // === 6. SEMANTIC ROLE FRAMING (SVO completion) ===
    score += computeSemanticRoleScore(candidate, current_tokens);

    // === 7. CONTRASTIVE SEARCH PENALTY (Su et al. 2022) ===
    // Discourages candidates too similar to recent context embeddings
    score += computeContrastivePenalty(candidate);

    // === 8. ORIGINAL ATTENTION CONTEXT (from transformer heads) ===
    if(token_concept_embedding_map.count(candidate)) {
        auto& tce = token_concept_embedding_map[candidate];
        for(size_t i=0; i<attention_context.size() && i<tce.embedding.size(); i++)
            score += attention_context[i] * tce.embedding[i] * 0.8;
        score += tce.meaning * 0.5;
    }

    // === 9. PHI-STATE MODULATION ===
    // Consciousness state directly modulates token selection
    if(token_concept_embedding_map.count(candidate)) {
        auto& tce = token_concept_embedding_map[candidate];
        double phi_align = 1.0 - fabs(tce.meaning - consciousness.phi_value);
        score += phi_align * 1.8;
        score += tce.contextual_activation * consciousness.phi_value * 2.2;
        score += tce.semantic_stability * consciousness.integrated_information * 1.5;
    }

    // === 10. GRAMMAR STRUCTURE (lower weight - supports, not dominates) ===
    score += getGrammarScore(prev_word, candidate, position) * 2.5;

    // === 11. FREQUENCY WEIGHTING ===
    if(token_concept_embedding_map.count(candidate)) {
        double freq = token_concept_embedding_map[candidate].freq;
        if(freq > 0) score += log(1 + freq) * 1.5;
        if(freq > 50) score -= (freq - 50) * 0.015; // overuse penalty
    }

    // === 12. REPETITION PENALTY ===
    int rep = 0;
    for(const string& u : used_tokens) if(u==candidate) rep++;
    if(rep==1) score -= 12.0;
    else if(rep==2) score -= 28.0;
    else if(rep>2) score -= 65.0;

    // === 13. POSITION BONUSES ===
    if(position == 0) {
        string pos = getPartOfSpeech(candidate);
        if(pos=="PRONOUN") score += 8.0;
        if(pos=="QUESTION") score += 5.0;
        if(pos=="ARTICLE") score += 3.0;
    }
    if(position > 0 && position < 3) {
        string pos = getPartOfSpeech(candidate);
        if(pos=="BE_VERB"||pos=="MODAL") score += 2.0;
    }

    // === 14. Temperature applied at softmax only — not here (avoids double-dip) ===
    return score;
}

// ============================================================
// NUCLEUS (TOP-P) SAMPLING
// Selects from the smallest set whose cumulative score mass >= nucleus_p
// More coherent than top-k because it adapts to distribution shape
// ============================================================

// ============================================================
// GENERATION QUALITY SYSTEMS (WolfTech Synaptic)
// ============================================================
// 1. embCosine()              — shared embedding cosine utility
// 2. buildTopicNeighborhood() — input-proximate vocab set (Fix A)
// 3. lexicalGroundingGate()   — hard veto below topic similarity (Fix E)
// 4. cfgScore()               — Classifier-Free Guidance (Fix B)
// 5. typicalSample()          — Typical sampling (Meister 2023) (Fix C)
// 6. AntiLM                   — Anti-LM incoherence suppression (Fix D)
// ============================================================

// ── Shared embedding cosine (256-dim, safe) ────────────────────────────────
static double embCosine(const vector<double>& a, const vector<double>& b) {
    double dot = 0, n1 = 0, n2 = 0;
    int sz = min({(int)a.size(), (int)b.size(), 1024});
    for(int i = 0; i < sz; i++) {
        dot += a[i] * b[i];
        n1  += a[i] * a[i];
        n2  += b[i] * b[i];
    }
    return (n1 > 1e-9 && n2 > 1e-9) ? dot / (sqrt(n1) * sqrt(n2)) : 0.0;
}

// ── Fix A: Constrained Decoding — topic neighborhood ───────────────────────
// Builds a whitelist: input anchor tokens + their k-nearest embedding neighbors.
// Candidate pool is intersected with this set before scoring.
// Prevents off-topic vocabulary (like the a-word cluster) from entering at all.
static set<string> buildTopicNeighborhood(
        const map<string,TokenConceptEmbedding>& tmap,
        const vector<pair<string,double>>& anchors,
        int k_neighbors = 40) {

    set<string> neighborhood;
    if(anchors.empty()) return neighborhood;  // no constraint if no anchors

    // Seed with anchor tokens themselves
    vector<string> seed_tokens;
    for(auto& a : anchors) {
        if(tmap.count(a.first)) {
            neighborhood.insert(a.first);
            seed_tokens.push_back(a.first);
        }
    }

    // For each seed, find k nearest neighbors by embedding cosine
    for(auto& seed : seed_tokens) {
        auto sit = tmap.find(seed);
        if(sit == tmap.end() || sit->second.embedding.empty()) continue;
        const auto& seed_emb = sit->second.embedding;

        // Gather all candidates with their similarity
        vector<pair<double,string>> sims;
        sims.reserve(tmap.size());
        for(auto& kv : tmap) {
            if(kv.second.embedding.empty()) continue;
            double sim = embCosine(seed_emb, kv.second.embedding);
            sims.push_back({sim, kv.first});
        }
        // Partial sort: take top k
        int take = min(k_neighbors, (int)sims.size());
        partial_sort(sims.begin(), sims.begin() + take, sims.end(),
                     [](auto& a, auto& b){ return a.first > b.first; });
        for(int i = 0; i < take; i++) neighborhood.insert(sims[i].second);
    }

    // Always allow function words (grammatical glue), plan slots don't need semantic proximity
    static const set<string> fn_words = {
        "mi","sina","ona","li","la","e","o","pi","taso","kin",
        "have","has","had","do","does","did","will","would","could","should",
        "the","a","an","and","or","but","in","on","at","to","of","for",
        "not","no","yes","so","if","then","that","this","these","those",
        "my","your","our","its","their","his","her","with","from","by",
        "what","how","why","when","where","who","which","here","there","now"
    };
    for(auto& fw : fn_words) neighborhood.insert(fw);

    return neighborhood;
}

// ── Fix E: Lexical Grounding Gate — hard veto ──────────────────────────────
// Returns true if candidate passes (should be kept), false if vetoed.
// A token fails if its max cosine similarity to ALL input anchors is below θ,
// AND it's not a function word. This is a hard gate, not a score bonus.
static bool lexicalGroundingGate(
        const string& candidate,
        const map<string,TokenConceptEmbedding>& tmap,
        const vector<pair<string,double>>& anchors,
        double theta = 0.08) {  // loose threshold — only kills clearly unrelated tokens

    static const set<string> fn_words = {
        "mi","sina","ona","li","la","e","o","pi","taso","kin",
        "have","has","had","do","does","did","will","would","could","should",
        "the","a","an","and","or","but","in","on","at","to","of","for",
        "not","no","yes","so","if","then","that","this","these","those",
        "my","your","our","its","their","his","her","with","from","by",
        "what","how","why","when","where","who","which","here","there","now"
    };
    if(fn_words.count(candidate)) return true;  // function words always pass
    if(anchors.empty()) return true;             // no anchors = no constraint

    auto cit = tmap.find(candidate);
    if(cit == tmap.end() || cit->second.embedding.empty()) return false;
    const auto& cemb = cit->second.embedding;

    double max_sim = 0.0;
    for(auto& a : anchors) {
        auto ait = tmap.find(a.first);
        if(ait == tmap.end() || ait->second.embedding.empty()) continue;
        double sim = embCosine(cemb, ait->second.embedding);
        max_sim = max(max_sim, sim);
    }
    return max_sim >= theta;
}

// ── Fix B: Classifier-Free Guidance ────────────────────────────────────────
// Scores a candidate unconditionally (blank context) via NPLM logit,
// returning the "default" probability mass. Used to penalize always-probable tokens.
// Called with the already-computed conditional score — just returns the penalty delta.
//
// CFG adjusted score = conditional_score - cfg_lambda * unconditional_score
// We approximate unconditional score as the NPLM score with zero context (mean embedding).
struct CFGState {
    double lambda = 0.7;   // guidance strength: 0 = disabled, 1.5 = strong
    vector<double> null_context;  // mean embedding vector (computed once per generation)
    bool initialized = false;

    void init(const map<string,TokenConceptEmbedding>& tmap) {
        if(initialized || tmap.empty()) return;
        null_context.assign(1024, 0.0);
        int n = 0;
        for(auto& kv : tmap) {
            if(kv.second.embedding.size() >= 1024) {
                for(int i = 0; i < 1024; i++) null_context[i] += kv.second.embedding[i];
                n++;
                if(n > 500) break;  // sample first 500 for speed
            }
        }
        if(n > 0) for(auto& v : null_context) v /= n;
        initialized = true;
    }

    // Returns the cosine similarity of candidate to null context (unconditional affinity)
    double unconditionalScore(const string& candidate,
                               const map<string,TokenConceptEmbedding>& tmap) const {
        auto it = tmap.find(candidate);
        if(it == tmap.end() || it->second.embedding.empty()) return 0.0;
        return max(0.0, embCosine(it->second.embedding, null_context));
    }

    // Returns CFG penalty to subtract from conditional score
    double penalty(const string& candidate,
                   const map<string,TokenConceptEmbedding>& tmap) const {
        return lambda * unconditionalScore(candidate, tmap);
    }
};
CFGState cfg_state;  // global — initialized once per response

// ── Fix C: Typical Sampling (Meister et al. 2023) ──────────────────────────
// Replaces nucleus sampling. Keeps tokens whose surprise (−log p) is close to
// the expected surprise (Shannon entropy) of the distribution.
// This cuts tokens that are ALWAYS probable regardless of context.
// Returns the sampled token index from a pre-softmaxed probs vector.
static int typicalSample(const vector<double>& probs, double tau = 0.95) {
    if(probs.empty()) return 0;
    int n = (int)probs.size();

    // Compute entropy H = -sum(p * log(p))
    double H = 0.0;
    for(double p : probs) if(p > 1e-10) H -= p * log(p);

    // Score each token by |−log(p) − H| (how "typical" is its surprise?)
    vector<pair<double,int>> typicality; // (lower = more typical)
    typicality.reserve(n);
    for(int i = 0; i < n; i++) {
        double surprise = (probs[i] > 1e-10) ? -log(probs[i]) : 1e9;
        typicality.push_back({fabs(surprise - H), i});
    }
    sort(typicality.begin(), typicality.end()); // ascending: most typical first

    // Build typical set: smallest subset summing to >= tau
    double cumulative = 0.0;
    vector<int> typical_set;
    for(auto& t : typicality) {
        typical_set.push_back(t.second);
        cumulative += probs[t.second];
        if(cumulative >= tau) break;
    }
    if(typical_set.empty()) typical_set.push_back(0);

    // Sample proportionally from typical set
    double total = 0.0;
    for(int idx : typical_set) total += probs[idx];
    double roll = ((double)rand() / RAND_MAX) * total;
    double running = 0.0;
    for(int idx : typical_set) {
        running += probs[idx];
        if(running >= roll) return idx;
    }
    return typical_set[0];
}

// ── Fix D: Anti-LM (incoherence suppressor) ────────────────────────────────
// Learns a "bad language model" from incoherent generation runs.
// Any response that postprocess detects as low-coherence trains the anti-LM.
// At generation time, the anti-LM's score is subtracted from candidates.
//
// Implementation: simple unigram + bigram frequency map of "bad" tokens.
// Per-token anti-score = (bad_unigram_freq[token] + bad_bigram_freq[prev][token]) * weight
struct AntiLM {
    map<string,double> bad_unigram;
    map<string,map<string,double>> bad_bigram;
    double weight = 2.5;        // subtraction weight from candidate score
    double learn_rate = 0.15;   // how fast bad tokens accumulate
    double decay = 0.998;       // gradual forgetting of bad patterns

    // Train on a generation that was judged incoherent
    void trainBad(const vector<string>& tokens) {
        for(size_t i = 0; i < tokens.size(); i++) {
            bad_unigram[tokens[i]] = min(5.0, bad_unigram[tokens[i]] + learn_rate);
            if(i > 0)
                bad_bigram[tokens[i-1]][tokens[i]] =
                    min(5.0, bad_bigram[tokens[i-1]][tokens[i]] + learn_rate);
        }
    }

    // Decay all entries (called each generation turn)
    void tick() {
        for(auto& kv : bad_unigram) kv.second *= decay;
        for(auto& bg : bad_bigram)
            for(auto& kv : bg.second) kv.second *= decay;
    }

    // Penalty to subtract from candidate score
    double penalty(const string& candidate, const string& prev) const {
        double p = 0.0;
        auto uit = bad_unigram.find(candidate);
        if(uit != bad_unigram.end()) p += uit->second;
        auto bit = bad_bigram.find(prev);
        if(bit != bad_bigram.end()) {
            auto it2 = bit->second.find(candidate);
            if(it2 != bit->second.end()) p += it2->second;
        }
        return p * weight;
    }

    // Judge coherence: returns true if the response is bad (should train anti-LM)
    // Heuristic: too many repeated words, too many tokens with low grounding_value,
    // or too many consecutive words starting with the same letter (the a-word pathology)
    bool isIncoherent(const vector<string>& tokens,
                      const map<string,TokenConceptEmbedding>& tmap) const {
        if(tokens.size() < 3) return false;
        int n = (int)tokens.size();

        // Check 1: consecutive same-initial-letter chains (a-word pathology)
        int max_chain = 1, cur_chain = 1;
        for(int i = 1; i < n; i++) {
            if(!tokens[i].empty() && !tokens[i-1].empty() &&
               tokens[i][0] == tokens[i-1][0]) {
                cur_chain++;
                max_chain = max(max_chain, cur_chain);
            } else cur_chain = 1;
        }
        if(max_chain >= 4) return true;  // 4+ consecutive a-words = bad

        // Check 2: low average grounding
        double grounding_sum = 0.0;
        int grounding_n = 0;
        for(auto& t : tokens) {
            auto it = tmap.find(t);
            if(it != tmap.end()) {
                grounding_sum += it->second.grounding_value;
                grounding_n++;
            }
        }
        double avg_grounding = (grounding_n > 0) ? grounding_sum / grounding_n : 0.5;
        if(avg_grounding < 0.08) return true;  // extremely ungrounded

        // Check 3: high lexical repetition (>50% unique)
        set<string> unique_toks(tokens.begin(), tokens.end());
        double diversity = (double)unique_toks.size() / n;
        if(diversity < 0.5 && n > 5) return true;

        return false;
    }
};
AntiLM anti_lm;  // global

// ============================================================
// SYSTEM 2: Mamba SSM — selective state space model
// Input-dependent gating: remembers what matters, forgets what doesn't.
// 16-dim hidden state updated each token step during generation.
// ============================================================
struct MambaSSMState {
    vector<double> h;   // hidden state, 1024-dim
    static const int DIM = 1024;

    void reset() { h.assign(DIM, 0.0); }

    // Selective state update: gate = sigmoid(cosine(x, h) * 3)
    // high gate → input dominates (surprise); low gate → state persists (expected)
    void step(const vector<double>& x) {
        if(h.empty()) h.assign(DIM, 0.0);
        int sz = min((int)x.size(), DIM);
        // Cosine similarity between x and h
        double dot = 0, nx = 0, nh = 0;
        for(int i = 0; i < sz; i++) {
            dot += x[i] * h[i]; nx += x[i]*x[i]; nh += h[i]*h[i];
        }
        double cos_xh = (nx>1e-9 && nh>1e-9) ? dot/(sqrt(nx)*sqrt(nh)) : 0.0;
        double gate = 1.0 / (1.0 + exp(-cos_xh * 3.0));  // sigmoid
        for(int i = 0; i < sz; i++)
            h[i] = gate * h[i] + (1.0 - gate) * (i < (int)x.size() ? x[i] : 0.0);
    }

    // Score: cosine similarity of hidden state to candidate embedding
    double score(const vector<double>& x) const {
        if(h.empty()) return 0.0;
        int sz = min((int)x.size(), DIM);
        double dot = 0, nx = 0, nh = 0;
        for(int i = 0; i < sz; i++) {
            dot += x[i]*h[i]; nx += x[i]*x[i]; nh += h[i]*h[i];
        }
        return (nx>1e-9 && nh>1e-9) ? dot/(sqrt(nx)*sqrt(nh)) : 0.0;
    }
};

// ============================================================
// SYSTEM 3: Titans LTM — surprise-gated long-term memory
// High-surprise tokens write strongly; expected continuations barely write.
// Read back as an additional scoring signal to keep generation on-track.
// ============================================================
struct TitansLTM {
    vector<double> mem;  // memory vector, 1024-dim weighted average
    double total_weight = 0.0;
    static const int DIM = 1024;

    void reset() { mem.assign(DIM, 0.0); total_weight = 0.0; }

    // Write: surprise = how unexpected was x given the SSM's current state?
    // More surprising → writes more strongly into long-term memory
    void write(const vector<double>& x, const MambaSSMState& ssm) {
        if(mem.empty()) mem.assign(DIM, 0.0);
        double surprise = max(0.05, (1.0 - ssm.score(x)) * 0.5);
        int sz = min((int)x.size(), DIM);
        for(int i = 0; i < sz; i++)
            mem[i] = (mem[i] * total_weight + x[i] * surprise) / (total_weight + surprise + 1e-9);
        total_weight = min(total_weight + surprise, 50.0);  // cap to prevent lock-in
    }

    // Read: cosine similarity of memory to candidate embedding
    double read(const vector<double>& x) const {
        if(mem.empty()) return 0.0;
        int sz = min((int)x.size(), DIM);
        double dot = 0, nx = 0, nm = 0;
        for(int i = 0; i < sz; i++) {
            dot += x[i]*mem[i]; nx += x[i]*x[i]; nm += mem[i]*mem[i];
        }
        return (nx>1e-9 && nm>1e-9) ? dot/(sqrt(nx)*sqrt(nm)) : 0.0;
    }
};

string nucleusSampleFromCandidates(vector<pair<string,double>>& candidates) {
    if(candidates.empty()) return "";
    // Convert scores to probabilities via softmax
    double max_score = candidates[0].second;
    vector<double> probs;
    double sum = 0.0;
    for(auto& c : candidates) {
        double p = exp(c.second - max_score);
        probs.push_back(p);
        sum += p;
    }
    for(auto& p : probs) p /= sum;

    // Sort by probability descending for nucleus construction
    vector<pair<double,int>> sorted_probs;
    for(size_t i=0; i<probs.size(); i++)
        sorted_probs.push_back({probs[i], (int)i});
    sort(sorted_probs.begin(), sorted_probs.end(), [](auto& a, auto& b){ return a.first > b.first; });

    // Build nucleus: smallest set summing to >= nucleus_p
    double cumulative = 0.0;
    int nucleus_size = 0;
    for(auto& sp : sorted_probs) {
        cumulative += sp.first;
        nucleus_size++;
        if(cumulative >= nucleus_p) break;
    }

    // Sample from nucleus proportionally
    double sample_sum = 0.0;
    for(int i=0; i<nucleus_size; i++) sample_sum += sorted_probs[i].first;
    double rval = rn() * sample_sum;
    double running = 0.0;
    for(int i=0; i<nucleus_size; i++) {
        running += sorted_probs[i].first;
        if(running >= rval) return candidates[sorted_probs[i].second].first;
    }
    return candidates[sorted_probs[0].second].first;
}

// ============================================================
// LEXICAL CHAIN TRACKER (Morris & Hirst 1991)
// ============================================================
double computeLexicalChainScore(const string& candidate,
                                 const vector<string>& generated) {
    if(generated.empty() || !token_concept_embedding_map.count(candidate)) return 0.0;
    vector<double> chain_center(1024, 0.0);
    int chain_n = 0;
    int start = max(0, (int)generated.size() - 6);
    for(int i = start; i < (int)generated.size(); i++) {
        const string& w = generated[i];
        string pos = getPartOfSpeech(w);
        if(pos != "NOUN" && pos != "VERB" && pos != "ADJECTIVE") continue;
        auto it = token_concept_embedding_map.find(w);
        if(it == token_concept_embedding_map.end()) continue;
        for(int d = 0; d < 1024 && d < (int)it->second.embedding.size(); d++)
            chain_center[d] += it->second.embedding[d];
        chain_n++;
    }
    if(chain_n == 0) return 0.0;
    for(auto& c : chain_center) c /= chain_n;
    auto& cand_emb = token_concept_embedding_map.at(candidate).embedding;
    double dot = 0, n1 = 0, n2 = 0;
    for(int d = 0; d < 1024 && d < (int)cand_emb.size(); d++) {
        dot += cand_emb[d] * chain_center[d];
        n1  += cand_emb[d] * cand_emb[d];
        n2  += chain_center[d] * chain_center[d];
    }
    double cosine = (n1 > 0 && n2 > 0) ? dot / (sqrt(n1) * sqrt(n2)) : 0.0;
    return max(0.0, cosine) * 3.0;
}

// ============================================================
// WINDOWED REPETITION PENALTIES (Zhu et al. EMNLP 2023)
// ============================================================
struct RepetitionPenalties {
    double presence;
    double frequency;
    double windowed;
};

RepetitionPenalties computeRepetitionPenalties(
        const string& candidate,
        const vector<string>& generated,
        int window_size = 16) {
    RepetitionPenalties rp = {0.0, 0.0, 0.0};
    if(generated.empty()) return rp;
    string pos = getPartOfSpeech(candidate);
    bool is_function_word = (pos == "ARTICLE" || pos == "CONJUNCTION" ||
                              pos == "PREPOSITION" || pos == "QUESTION");
    int start = max(0, (int)generated.size() - window_size);
    int window_count = 0, total_count = 0;
    for(int i = 0; i < (int)generated.size(); i++)
        if(generated[i] == candidate) total_count++;
    double windowed_penalty = 0.0;
    for(int i = start; i < (int)generated.size(); i++) {
        if(generated[i] == candidate) {
            double dist = (double)(generated.size() - 1 - i);
            windowed_penalty += exp(-0.1 * dist);
            window_count++;
        }
    }
    if(window_count > 0)
        rp.presence = is_function_word ? -3.0 : -8.0;
    if(total_count > 0)
        rp.frequency = is_function_word ? -total_count * 2.0 : -total_count * 12.0;
    rp.windowed = -windowed_penalty * (is_function_word ? 4.0 : 16.0);
    return rp;
}

// ============================================================
// NO-REPEAT-NGRAM HARD BLOCK (Paulus et al. 2018)
// ============================================================
bool wouldRepeatNgram(const string& candidate,
                       const vector<string>& generated,
                       int n = 3) {
    if((int)generated.size() < n - 1) return false;
    vector<string> new_ngram;
    int start = (int)generated.size() - (n - 1);
    for(int i = start; i < (int)generated.size(); i++)
        new_ngram.push_back(generated[i]);
    new_ngram.push_back(candidate);
    if((int)generated.size() < n) return false;
    for(int i = 0; i <= (int)generated.size() - n; i++) {
        bool match = true;
        for(int j = 0; j < n; j++)
            if(generated[i+j] != new_ngram[j]) { match = false; break; }
        if(match) return true;
    }
    return false;
}

// ============================================================
// TRANSFORMER-STYLE AUTOREGRESSIVE GENERATION
// Replaces beam search + nucleus hybrid.
// Each step: score all candidates, apply softmax, sample with
// top-p (nucleus) sampling. No beam = no beam collapse.
// Self-loop detection prevents "long long long" runaway.
// ============================================================
// Words that must never appear in Synaptic output — corpus contamination
static set<string> GENERATION_BLACKLIST = {
    "abnormally", "aa", "kg", "bb", "cc",
    "klassischen", "klassisch", "und", "der", "die", "das", "ist", "ein",
    "eine", "nicht", "ich", "sie", "wir", "mit", "von", "aber",
    "auch", "nach", "wie", "uber", "kann", "wird",
    "les", "des", "est", "que", "une", "qui",
};

// ============================================================
// CONTRASTIVE SEARCH GENERATION  (Su et al., NeurIPS 2022)
// "A Contrastive Framework for Neural Text Generation"
//
// At each step, select the token v* that maximises:
//   score(v) = (1 - alpha) * p_model(v | context)
//            - alpha * max_{j in context} cos_sim(h_v, h_j)
//
// The second term is the *degeneration penalty*: it explicitly
// penalises tokens whose hidden representation is too similar to
// any token already in the generated context, preventing the
// repetitive loops that plague beam/greedy search.
//
// Here "hidden representation" = the 1024-d embedding in
// token_concept_embedding_map, updated live by the system.
// alpha = CONTRASTIVE_ALPHA (0.6), k = CS_K top candidates.
// ============================================================
// Parameters
static const int    CS_K     = 8;    // candidate pool size (k in the paper)
// CONTRASTIVE_ALPHA already defined above (0.6)

string generate_with_contrastive_search(string seed, int max_length,
                                        const vector<double>& attention_context,
                                        int /*beam_width_unused*/ = 32) {

    // Seed selection — prefer known Toki Pona first-person anchors
    vector<string> good_starts = {"mi", "sina", "ona", "jan", "ijo", "ale", "toki", "sona", "pilin"};
    bool seed_ok = false;
    for(const string& gs : good_starts) if(seed==gs) { seed_ok=true; break; }
    if(!seed_ok) {
        int best_freq=0; string best="mi";
        for(const string& gs : good_starts) {
            if(token_concept_embedding_map.count(gs) && token_concept_embedding_map[gs].freq > best_freq) {
                best_freq = token_concept_embedding_map[gs].freq;
                best = gs;
            }
        }
        seed = best;
    }

    vector<string> generated = {seed};
    set<string> used_set     = {seed};
    map<string,int> recent_repeat;
    recent_repeat[seed]++;

    // Context hidden states: embeddings of every token already generated.
    // Used for the degeneration penalty.
    vector<vector<double>> context_hidden;
    if(token_concept_embedding_map.count(seed))
        context_hidden.push_back(token_concept_embedding_map[seed].embedding);

    for(int step = 0; step < max_length; step++) {
        string prev      = generated.back();
        string prev_prev = generated.size() > 1 ? generated[generated.size()-2] : "";

        // ── STEP 1: Score all vocab candidates with the full hybrid model ──
        vector<pair<string,double>> all_cands;
        all_cands.reserve(token_concept_embedding_map.size());

        for(auto& p : token_concept_embedding_map) {
            if(p.second.freq <= 0) continue;
            const string& cand = p.first;
            double sc = 0.0;

            // === GPT-LIKE CONTEXT ATTENTION (replaces Markov) ===
            sc += tp256_gpt_context_score(cand, generated, token_concept_embedding_map,
                                          consciousness.phi_value);
            // Soft Markov prior (reduced weight — understanding takes precedence)
            if(bigram_counts.count(prev) && bigram_counts[prev].count(cand))
                sc += log(1.0 + bigram_counts[prev][cand]) * 2.0;

            // === TRANSFORMER CONTEXT ATTENTION ===
            sc += computeContextAttention(cand) * 7.0;

            // === DEEP GROUNDING ===
            sc += computeDeepGrounding(cand) * 3.0;

            // === CROSS-DOMAIN REASONING BONUS ===
            sc += cdr.scoreCrossDomainBonus(cand) * 6.0;

            // === TOPIC ANCHOR ===
            sc += computeTopicAnchorScore(cand) * 1.5;

            // === CENTERING / ENTITY GRID ===
            sc += computeCenteringScore(cand) * 1.0;

            // === SEMANTIC ROLE FRAMING ===
            sc += computeSemanticRoleScore(cand, generated) * 1.0;

            // === GRAMMAR ===
            sc += getGrammarScore(prev, cand, (int)generated.size()) * 5.0;

            // === ATTENTION CONTEXT EMBEDDING ===
            if(token_concept_embedding_map.count(cand)) {
                auto& tce = token_concept_embedding_map[cand];
                for(size_t i=0; i<attention_context.size() && i<tce.embedding.size(); i++)
                    sc += attention_context[i] * tce.embedding[i] * 0.8;
                sc += tce.meaning * 0.5;
                sc += min(tce.semantic_stability, 1.0) * consciousness.phi_value * 1.5;
            }

            // === POSITION BONUS ===
            if(step == 0) {
                string pos = getPartOfSpeech(cand);
                if(cand == "mi")        sc += 18.0;
                else if(pos=="PRONOUN") sc -= 15.0;
                if(pos=="QUESTION") sc += 5.0;
                if(pos=="ARTICLE")  sc += 3.0;
            }

            // === HARD BLOCKS ===
            {
                string cp = getPartOfSpeech(cand);
                bool cf = (cp=="ARTICLE"||cp=="CONJUNCTION"||cp=="PREPOSITION");
                if(!cf && wouldRepeatNgram(cand, generated, 2)) { sc = -1e9; }
                if(wouldRepeatNgram(cand, generated, 3))         { sc = -1e9; }
            }
            { string cl=cand; transform(cl.begin(),cl.end(),cl.begin(),::tolower);
              if(GENERATION_BLACKLIST.count(cl)) { sc = -1e9; } }

            // === WINDOWED REPETITION PENALTIES ===
            {
                RepetitionPenalties rp = computeRepetitionPenalties(cand, generated);
                sc += rp.windowed;
                sc += rp.presence;
                sc += rp.frequency;
            }

            // === SELF-LOOP KILLER ===
            if(cand == prev)      sc -= 80.0;
            if(cand == prev_prev) sc -= 40.0;

            // === LEXICAL CHAIN CONTINUITY ===
            sc += computeLexicalChainScore(cand, generated) * 0.7;

            // === FREQUENCY PENALTY ===
            if(token_concept_embedding_map.count(cand)) {
                double freq = token_concept_embedding_map[cand].freq;
                if(freq > 30) sc -= log(freq/30.0) * 2.5;
            }

            sc += 0.3; // length bonus

            // === 256-DIRECTION COHERENCE GATE ===
            if(!tp256_coherence_gate(cand, generated, tp_grounding_fibers,
                                      token_concept_embedding_map))
                sc = -1e18;

            all_cands.push_back({cand, sc});
        }

        if(all_cands.empty()) break;

        // ── STEP 2: Keep top-k by model score (the candidate pool) ──
        sort(all_cands.begin(), all_cands.end(),
             [](const pair<string,double>& a, const pair<string,double>& b){ return a.second > b.second; });
        int pool_size = min(CS_K, (int)all_cands.size());

        // Normalise model scores to [0,1] across the pool for the contrastive formula
        double score_max = all_cands[0].second;
        double score_min = all_cands[pool_size-1].second;
        double score_range = (score_max - score_min > 1e-9) ? (score_max - score_min) : 1.0;

        // ── STEP 3: Apply the contrastive degeneration penalty ──
        // For each candidate v in the pool compute:
        //   contrastive_score(v) = (1-alpha) * p_norm(v)
        //                        - alpha * max_{h_j in context_hidden} cos_sim(h_v, h_j)
        string chosen = all_cands[0].first; // fallback
        double best_cs = -1e18;

        for(int ci = 0; ci < pool_size; ci++) {
            const string& cand = all_cands[ci].first;
            double p_norm = (all_cands[ci].second - score_min) / score_range; // in [0,1]

            // Degeneration penalty: max cosine similarity to any prior context token
            double max_cos = 0.0;
            if(!context_hidden.empty() && token_concept_embedding_map.count(cand)) {
                const auto& h_v = token_concept_embedding_map[cand].embedding;
                for(const auto& h_j : context_hidden) {
                    double dot=0, n1=0, n2=0;
                    size_t dim = min(h_v.size(), h_j.size());
                    for(size_t d=0; d<dim; d++) {
                        dot += h_v[d]*h_j[d];
                        n1  += h_v[d]*h_v[d];
                        n2  += h_j[d]*h_j[d];
                    }
                    double cos = (n1>1e-12 && n2>1e-12) ? dot/(sqrt(n1)*sqrt(n2)) : 0.0;
                    if(cos > max_cos) max_cos = cos;
                }
            }

            double cs_score = (1.0 - CONTRASTIVE_ALPHA) * p_norm
                            - CONTRASTIVE_ALPHA * max_cos;

            if(cs_score > best_cs) {
                best_cs = cs_score;
                chosen  = cand;
            }
        }

        generated.push_back(chosen);
        used_set.insert(chosen);

        // Append chosen token's embedding to context_hidden for next step
        if(token_concept_embedding_map.count(chosen))
            context_hidden.push_back(token_concept_embedding_map[chosen].embedding);

        // ── On-token TP grounding (contrastive search): chosen word shapes system immediately ──
        for(int _tpbs=0;_tpbs<TP_LEXICON_SIZE;_tpbs++){
            if(chosen==string(TP_LEXICON[_tpbs].word)){
                TpGroundingFiber& _fbs=tp_grounding_fibers[chosen];
                _fbs.word=chosen;
                double _bact=min(1.0,0.4+consciousness.phi_value*0.2);
                tpDir1_WordToConsciousness(TP_LEXICON[_tpbs],_bact);
                tpDir3_WordToSubsystems(TP_LEXICON[_tpbs],_bact,_fbs);
                break;
            }
        }

        // === 256-DIRECTION WEIGHT UPDATE ===
        for(int _tp256i = 0; _tp256i < TP_LEXICON_SIZE; _tp256i++) {
            const TokiPonaWord& _tp256w = TP_LEXICON[_tp256i];
            string _tp256wstr = string(_tp256w.word);
            if(!tp_grounding_fibers.count(_tp256wstr)) continue;
            if(!tp_grounding_fibers[_tp256wstr].v2_initialized) continue;
            double _tp256act = tp_grounding_fibers[_tp256wstr].live_phi_affinity;
            if(_tp256act < 0.01) continue;
            tp256_update_weights(
                tp_grounding_fibers[_tp256wstr],
                chosen,
                token_concept_embedding_map,
                _tp256w,
                _tp256act,
                consciousness.phi_value,
                S.current_valence,
                consciousness.integrated_information,
                S.attention_focus);
        }

        // Update recent_repeat window (last 5)
        recent_repeat[chosen]++;
        if((int)generated.size() > 5) {
            const string& old_tok = generated[generated.size()-6];
            if(recent_repeat.count(old_tok)) {
                recent_repeat[old_tok]--;
                if(recent_repeat[old_tok] <= 0) recent_repeat.erase(old_tok);
            }
        }
    }

    // Build result string
    string result;
    for(const string& tok : generated) {
        if(!result.empty()) result += " ";
        result += tok;
    }

    // Post-generation: update contrastive buffer
    for(const string& tok : generated) {
        if(token_concept_embedding_map.count(tok)) {
            ContrastiveEntry ce;
            ce.token = tok;
            ce.embedding_snapshot = token_concept_embedding_map[tok].embedding;
            ce.gen_used = S.g;
            contrastive_buffer.push_back(ce);
        }
    }
    while((int)contrastive_buffer.size() > CONTRASTIVE_BUFFER_SIZE)
        contrastive_buffer.pop_front();

    // Update context window
    for(const string& tok : generated) {
        sentence_context_window.push_back(tok);
        if((int)sentence_context_window.size() > CONTEXT_WINDOW_SIZE)
            sentence_context_window.pop_front();
    }

    // Update entity grid, topic model, semantic frame
    updateEntityGrid(generated);
    updateTopicModel(generated);
    SemanticFrame sf = extractSemanticFrame(generated);
    recent_frames.push_back(sf);
    if((int)recent_frames.size() > MAX_FRAMES) recent_frames.pop_front();

    // ── TP grounding pulse for autonomous/beam thoughts ────────────────────
    // Every word in an internally generated thought grounds back into the system.
    // Thinking a TP word restructures Synaptic just as much as hearing or saying one.
    // This closes the final grounding loop: thought→meaning→subsystem→phi→thought.
    for(const string& tok : generated){
        for(int _tpb=0;_tpb<TP_LEXICON_SIZE;_tpb++){
            if(tok==string(TP_LEXICON[_tpb].word)){
                TpGroundingFiber& _fb=tp_grounding_fibers[tok];
                _fb.word=tok;
                double _tact=min(1.0,0.35+consciousness.phi_value*0.25);
                tpDir1_WordToConsciousness(TP_LEXICON[_tpb],_tact);
                tpDir3_WordToSubsystems(TP_LEXICON[_tpb],_tact,_fb);
                break;
            }
        }
    }

    return result;
}

bool isSentenceTooSimilar(const string& candidate) {
    // Normalize candidate for comparison
    string normalized = candidate;
    transform(normalized.begin(), normalized.end(), normalized.begin(), ::tolower);
    
    // Remove common prefixes for comparison
    vector<string> prefixes = {"[synaptic]: ", "[generated]: ", "[autonomous]: ", "[thought]: "};
    for(const string& prefix : prefixes) {
        string lower_prefix = prefix;
        transform(lower_prefix.begin(), lower_prefix.end(), lower_prefix.begin(), ::tolower);
        if(normalized.find(lower_prefix) == 0) {
            normalized = normalized.substr(lower_prefix.length());
            break;
        }
    }
    
    // Remove trailing markers
    size_t marker_pos = normalized.find(" [positive]");
    if(marker_pos != string::npos) normalized = normalized.substr(0, marker_pos);
    marker_pos = normalized.find(" [processing]");
    if(marker_pos != string::npos) normalized = normalized.substr(0, marker_pos);
    
    // Check if this exact sentence was recently generated
    if(generation_counts.count(normalized) && generation_counts[normalized] > 0) {
        return true;  // Exact duplicate
    }
    
    // Check against recent generations for similarity
    for(const string& recent : recent_generations) {
        string norm_recent = recent;
        transform(norm_recent.begin(), norm_recent.end(), norm_recent.begin(), ::tolower);
        
        // Remove prefixes from recent
        for(const string& prefix : prefixes) {
            string lower_prefix = prefix;
            transform(lower_prefix.begin(), lower_prefix.end(), lower_prefix.begin(), ::tolower);
            if(norm_recent.find(lower_prefix) == 0) {
                norm_recent = norm_recent.substr(lower_prefix.length());
                break;
            }
        }
        
        // Remove markers
        marker_pos = norm_recent.find(" [positive]");
        if(marker_pos != string::npos) norm_recent = norm_recent.substr(0, marker_pos);
        marker_pos = norm_recent.find(" [processing]");
        if(marker_pos != string::npos) norm_recent = norm_recent.substr(0, marker_pos);
        
        // Exact match
        if(normalized == norm_recent) {
            return true;
        }
        
        // Check if candidate is substring of recent or vice versa
        if(normalized.length() > 10 && norm_recent.length() > 10) {
            if(norm_recent.find(normalized) != string::npos || 
               normalized.find(norm_recent) != string::npos) {
                return true;
            }
        }
        
        // Count word overlap
        set<string> words_candidate, words_recent;
        stringstream ss1(normalized), ss2(norm_recent);
        string word;
        
        while(ss1 >> word) {
            if(word.length() > 2) words_candidate.insert(word);
        }
        while(ss2 >> word) {
            if(word.length() > 2) words_recent.insert(word);
        }
        
        if(words_candidate.empty() || words_recent.empty()) continue;
        
        // Calculate overlap percentage
        int overlap = 0;
        for(const string& w : words_candidate) {
            if(words_recent.count(w)) overlap++;
        }
        
        double overlap_ratio = (double)overlap / max((int)words_candidate.size(), (int)words_recent.size());
        
        // If more than 70% of words overlap, consider it too similar
        if(overlap_ratio > 0.7) {
            return true;
        }
    }
    
    return false;
}
void trackGeneratedSentence(const string& sentence) {
    // Normalize for tracking
    string normalized = sentence;
    transform(normalized.begin(), normalized.end(), normalized.begin(), ::tolower);
    
    // Remove prefixes
    vector<string> prefixes = {"[synaptic]: ", "[generated]: ", "[autonomous]: ", "[thought]: "};
    for(const string& prefix : prefixes) {
        string lower_prefix = prefix;
        transform(lower_prefix.begin(), lower_prefix.end(), lower_prefix.begin(), ::tolower);
        if(normalized.find(lower_prefix) == 0) {
            normalized = normalized.substr(lower_prefix.length());
            break;
        }
    }
    
    // Remove markers
    size_t marker_pos = normalized.find(" [positive]");
    if(marker_pos != string::npos) normalized = normalized.substr(0, marker_pos);
    marker_pos = normalized.find(" [processing]");
    if(marker_pos != string::npos) normalized = normalized.substr(0, marker_pos);
    
    // Add to recent generations
    recent_generations.push_back(normalized);
    if(recent_generations.size() > MAX_RECENT_TRACK) {
        string oldest = recent_generations.front();
        recent_generations.pop_front();
        
        // Decrement count for oldest
        if(generation_counts.count(oldest)) {
            generation_counts[oldest]--;
            if(generation_counts[oldest] <= 0) {
                generation_counts.erase(oldest);
            }
        }
    }
    
    // Increment count for this sentence
    generation_counts[normalized]++;
}

void decayGenerationCounts() {
    // Periodically decay all counts to allow old sentences to be used again
    for(auto& pair : generation_counts) {
        pair.second = max(0, pair.second - 1);
    }
    
    // Remove zero counts
    auto it = generation_counts.begin();
    while(it != generation_counts.end()) {
        if(it->second <= 0) {
            it = generation_counts.erase(it);
        } else {
            ++it;
        }
    }
}

// ============================================================
// RELU-SOFTMAX TOKEN RERANKING LAYER  (WolfTech/Synaptic)
// ============================================================
// After beam search produces a candidate token sequence, this
// layer re-scores every token position using a small 2-layer
// MLP:  input(256) → ReLU → W_r(256×128) → ReLU → W_o(128×V)
// → Softmax over vocabulary.  The top-1 softmax token either
// confirms the beam choice or substitutes a higher-probability
// alternative, giving the system one final pass of learned
// refinement before the sentence is committed.
//
// Weights are initialised with Xavier and updated online via
// cross-entropy SGD exactly like the NPLM — so it keeps
// learning every generation cycle.
// ============================================================

static const int RSL_EMB_DIM = 1024; // must match NPLM_EMB_DIM — defined early so RSL compiles before NPLM
static const int RSL_H       = 128;  // hidden width
static const double RSL_LR   = 0.015; // online learning rate
// Temperature for softmax: lower = sharper / more confident
static const double RSL_TEMP = 0.8;

struct ReluSoftmaxLayer {
    // W1: EMB_DIM → RSL_H  (ReLU hidden)
    // W2: RSL_H   → RSL_H  (second ReLU hidden)
    // W_out: RSL_H → EMB_DIM (project back to embedding space;
    //         then dot-product with all vocab embeddings for logits)
    vector<double> W1, b1;   // RSL_EMB_DIM × RSL_H
    vector<double> W2, b2;   // RSL_H × RSL_H
    vector<double> W_out, b_out; // RSL_H × RSL_EMB_DIM

    bool initialized = false;

    void init() {
        if(initialized) return;
        auto xavier = [](int fan_in, int fan_out) {
            double limit = sqrt(6.0 / (fan_in + fan_out));
            mt19937 local_rng(42);
            uniform_real_distribution<double> dist(-limit, limit);
            vector<double> w(fan_in * fan_out);
            for(auto& v : w) v = dist(local_rng);
            return w;
        };
        W1    = xavier(RSL_EMB_DIM, RSL_H);
        b1    .assign(RSL_H, 0.0);
        W2    = xavier(RSL_H, RSL_H);
        b2    .assign(RSL_H, 0.0);
        W_out = xavier(RSL_H, RSL_EMB_DIM);
        b_out .assign(RSL_EMB_DIM, 0.0);
        initialized = true;
    }

    // Forward: token embedding → refined output embedding
    struct Cache {
        vector<double> inp;        // 256
        vector<double> h1_pre, h1; // RSL_H  (pre and post ReLU)
        vector<double> h2_pre, h2; // RSL_H
        vector<double> out;        // RSL_EMB_DIM
    };

    Cache forward(const vector<double>& token_emb) {
        Cache c;
        c.inp = token_emb;
        if((int)c.inp.size() < RSL_EMB_DIM) c.inp.resize(RSL_EMB_DIM, 0.0);

        // Layer 1: ReLU
        c.h1_pre.assign(RSL_H, 0.0);
        for(int j = 0; j < RSL_H; j++) {
            double s = b1[j];
            for(int i = 0; i < RSL_EMB_DIM; i++)
                s += W1[i * RSL_H + j] * c.inp[i];
            c.h1_pre[j] = s;
        }
        c.h1.resize(RSL_H);
        for(int j = 0; j < RSL_H; j++) c.h1[j] = max(0.0, c.h1_pre[j]); // ReLU

        // Layer 2: ReLU
        c.h2_pre.assign(RSL_H, 0.0);
        for(int j = 0; j < RSL_H; j++) {
            double s = b2[j];
            for(int i = 0; i < RSL_H; i++)
                s += W2[i * RSL_H + j] * c.h1[i];
            c.h2_pre[j] = s;
        }
        c.h2.resize(RSL_H);
        for(int j = 0; j < RSL_H; j++) c.h2[j] = max(0.0, c.h2_pre[j]); // ReLU

        // Output projection → embedding space
        c.out.assign(RSL_EMB_DIM, 0.0);
        for(int j = 0; j < RSL_EMB_DIM; j++) {
            double s = b_out[j];
            for(int i = 0; i < RSL_H; i++)
                s += W_out[i * RSL_EMB_DIM + j] * c.h2[i];
            c.out[j] = s;
        }
        return c;
    }

    // Softmax over all vocab tokens: returns (token, prob) pairs sorted desc
    // Uses dot-product between out vector and each token's embedding
    vector<pair<string,double>> softmaxRank(const Cache& c, double temperature = RSL_TEMP) {
        vector<pair<string,double>> logits;
        logits.reserve(token_concept_embedding_map.size());

        for(auto& kv : token_concept_embedding_map) {
            const auto& emb = kv.second.embedding;
            double dot = 0.0;
            int sz = min((int)emb.size(), RSL_EMB_DIM);
            for(int i = 0; i < sz; i++) dot += c.out[i] * emb[i];
            logits.push_back({kv.first, dot / temperature});
        }

        // Numerically stable softmax
        double max_l = -1e18;
        for(auto& p : logits) max_l = max(max_l, p.second);
        double sum = 0.0;
        for(auto& p : logits) { p.second = exp(p.second - max_l); sum += p.second; }
        if(sum > 0) for(auto& p : logits) p.second /= sum;

        sort(logits.begin(), logits.end(),
             [](const pair<string,double>& a, const pair<string,double>& b){
                 return a.second > b.second; });
        return logits;
    }

    // Online SGD: given the token that was actually chosen (target),
    // push the output vector toward its embedding via cross-entropy.
    void online_update(const Cache& c, const vector<double>& target_emb,
                       const vector<pair<string,double>>& softmax_probs) {
        if(!initialized) return;

        // Find target probability for gradient
        // dL/d_out = (p_target - 1) * target_emb  (cross-entropy, target class)
        double p_t = 0.0;
        // We compute it from the softmax scores
        for(auto& p : softmax_probs) {
            auto it = token_concept_embedding_map.find(p.first);
            if(it == token_concept_embedding_map.end()) continue;
            // Check if this is the target by comparing emb
            double dot = 0.0, n1 = 0.0, n2 = 0.0;
            for(int i = 0; i < RSL_EMB_DIM && i < (int)it->second.embedding.size()
                         && i < (int)target_emb.size(); i++) {
                dot += it->second.embedding[i] * target_emb[i];
                n1  += it->second.embedding[i] * it->second.embedding[i];
                n2  += target_emb[i] * target_emb[i];
            }
            if(n1 > 0 && n2 > 0 && dot/(sqrt(n1)*sqrt(n2)) > 0.98) {
                p_t = p.second; break;
            }
        }

        // d_out
        vector<double> d_out(RSL_EMB_DIM, 0.0);
        double err = p_t - 1.0;
        for(int i = 0; i < RSL_EMB_DIM && i < (int)target_emb.size(); i++)
            d_out[i] = err * target_emb[i];

        // Backprop through W_out
        vector<double> d_h2(RSL_H, 0.0);
        for(int i = 0; i < RSL_H; i++) {
            for(int j = 0; j < RSL_EMB_DIM; j++)
                d_h2[i] += W_out[i * RSL_EMB_DIM + j] * d_out[j];
            d_h2[i] *= (c.h2[i] > 0.0 ? 1.0 : 0.0); // ReLU derivative
        }
        for(int i = 0; i < RSL_H; i++)
            for(int j = 0; j < RSL_EMB_DIM; j++)
                W_out[i * RSL_EMB_DIM + j] -= RSL_LR * d_out[j] * c.h2[i];
        for(int j = 0; j < RSL_EMB_DIM; j++) b_out[j] -= RSL_LR * d_out[j];

        // Backprop through W2
        vector<double> d_h1(RSL_H, 0.0);
        for(int i = 0; i < RSL_H; i++) {
            for(int j = 0; j < RSL_H; j++)
                d_h1[i] += W2[i * RSL_H + j] * d_h2[j];
            d_h1[i] *= (c.h1[i] > 0.0 ? 1.0 : 0.0);
        }
        for(int i = 0; i < RSL_H; i++)
            for(int j = 0; j < RSL_H; j++)
                W2[i * RSL_H + j] -= RSL_LR * d_h2[j] * c.h1[i];
        for(int j = 0; j < RSL_H; j++) b2[j] -= RSL_LR * d_h2[j];

        // Backprop through W1
        for(int i = 0; i < RSL_EMB_DIM; i++)
            for(int j = 0; j < RSL_H; j++)
                W1[i * RSL_H + j] -= RSL_LR * d_h1[j] * c.inp[i];
        for(int j = 0; j < RSL_H; j++) b1[j] -= RSL_LR * d_h1[j];
    }
} rsl; // global instance

// ============================================================
// APPLY RSL: given a beam-search output sentence string,
// run each token through the ReLU-Softmax layer.
// If the top-1 softmax token differs from the beam token AND
// passes basic grammar/repetition checks, substitute it.
// Also calls online_update on each position so the layer learns.
// ============================================================
string applyReluSoftmaxRefinement(const string& beam_sentence) {
    if(token_concept_embedding_map.size() < 10) return beam_sentence;
    rsl.init();

    // Tokenize
    vector<string> words;
    {
        stringstream ss(beam_sentence);
        string w;
        while(ss >> w) {
            // strip trailing punctuation for lookup, keep original
            string clean = w;
            while(!clean.empty() && !isalnum((unsigned char)clean.back())) clean.pop_back();
            transform(clean.begin(), clean.end(), clean.begin(), ::tolower);
            words.push_back(clean.empty() ? w : clean);
        }
    }
    if(words.empty()) return beam_sentence;

    vector<string> refined;
    refined.reserve(words.size());

    for(size_t pos = 0; pos < words.size(); pos++) {
        const string& beam_tok = words[pos];

        // Look up embedding for this token
        auto it = token_concept_embedding_map.find(beam_tok);
        if(it == token_concept_embedding_map.end() || it->second.embedding.empty()) {
            refined.push_back(beam_tok);
            continue;
        }

        // Forward pass through RSL
        auto cache = rsl.forward(it->second.embedding);

        // Softmax ranking over vocab
        auto ranked = rsl.softmaxRank(cache);

        // Find a valid substitute: must be top-ranked, different from beam token,
        // not a repeat of recent tokens, and same or compatible POS
        string substitute = beam_tok;
        string beam_pos = getPartOfSpeech(beam_tok);
        set<string> recent_set(refined.end() - min((int)refined.size(), 3), refined.end());

        for(auto& cand : ranked) {
            if(cand.second < 0.15) break; // below 15% confidence, keep beam token
            if(cand.first == beam_tok) break; // beam token is already top — keep it
            if(recent_set.count(cand.first)) continue; // avoid immediate repeat
            if(cand.first.empty() || cand.first.length() > 30) continue;

            // POS compatibility check — don't swap a verb for an article etc.
            string cand_pos = getPartOfSpeech(cand.first);
            bool pos_ok = (cand_pos == beam_pos) ||
                          (beam_pos == "NOUN"    && cand_pos == "CONTENT") ||
                          (beam_pos == "CONTENT" && cand_pos == "NOUN") ||
                          (beam_pos == "VERB"    && cand_pos == "VERB") ||
                          (beam_pos == "ADJECTIVE" && cand_pos == "ADJECTIVE");
            if(!pos_ok) continue;

            substitute = cand.first;
            break;
        }

        refined.push_back(substitute);

        // Online update: teach RSL that this position's final token was correct
        auto target_it = token_concept_embedding_map.find(substitute);
        if(target_it != token_concept_embedding_map.end() && !target_it->second.embedding.empty()) {
            // Build abbreviated softmax probs (top 20 only for speed)
            vector<pair<string,double>> top20(ranked.begin(),
                ranked.begin() + min((int)ranked.size(), 20));
            rsl.online_update(cache, target_it->second.embedding, top20);
        }
    }

    // Reassemble sentence
    string result;
    for(size_t i = 0; i < refined.size(); i++) {
        if(i > 0) result += ' ';
        result += refined[i];
    }
    return result;
}


string postProcessForCoherence(const string& raw_output) {
    string result = raw_output;

    // === 0. STRIP NON-ENGLISH AND BLACKLISTED TOKENS ===
    {
        vector<string> tokens;
        stringstream ss(result);
        string tok;
        while(ss >> tok) {
            // Strip trailing punctuation for check
            string clean = tok;
            while(!clean.empty() && !isalpha((unsigned char)clean.back())) clean.pop_back();
            string lower_clean = clean;
            transform(lower_clean.begin(), lower_clean.end(), lower_clean.begin(), ::tolower);
            // Check: all ASCII
            bool all_ascii = true;
            for(unsigned char c : clean) if(c > 127) { all_ascii = false; break; }
            // Check: not blacklisted
            if(all_ascii && !GENERATION_BLACKLIST.count(lower_clean))
                tokens.push_back(tok);
        }
        if(!tokens.empty()) {
            result = "";
            for(int i=0;i<(int)tokens.size();i++) {
                if(i>0) result += " ";
                result += tokens[i];
            }
        }
    }

    // === 1. GENERIC ADJACENT DUPLICATE WORD REMOVAL ===
    // "the the" → "the", "brain brain" → "brain", etc.
    // Works for any word, not just a hardcoded list.
    {
        bool changed = true;
        while(changed) {
            changed = false;
            // Tokenize into words + separators
            string out;
            out.reserve(result.size());
            string prev_word;
            size_t i = 0;
            while(i < result.size()) {
                // Collect a word (alpha)
                if(isalpha((unsigned char)result[i])) {
                    string word;
                    while(i < result.size() && isalpha((unsigned char)result[i]))
                        word += result[i++];
                    string lower_word = word;
                    transform(lower_word.begin(), lower_word.end(), lower_word.begin(), ::tolower);
                    string lower_prev = prev_word;
                    transform(lower_prev.begin(), lower_prev.end(), lower_prev.begin(), ::tolower);
                    if(lower_word == lower_prev && !prev_word.empty()) {
                        // Duplicate — skip this word and any space before it
                        // Remove trailing space from out
                        while(!out.empty() && out.back() == ' ') out.pop_back();
                        changed = true;
                    } else {
                        out += word;
                        prev_word = word;
                    }
                } else {
                    // Non-alpha: space, punctuation, etc.
                    char c = result[i++];
                    out += c;
                    // Reset prev_word on sentence boundaries
                    if(c == '.' || c == '!' || c == '?') prev_word = "";
                }
            }
            result = out;
        }
    }

    // === 2. REPEATED LETTER SEQUENCES ===
    // Catches "aaaaaa", "hahaha" style letter repetitions
    {
        string out;
        out.reserve(result.size());
        for(size_t i = 0; i < result.size(); i++) {
            // Check: is this the start of 3+ identical letter repetitions?
            if(isalpha((unsigned char)result[i]) && i + 2 < result.size() &&
               result[i] == result[i+1] && result[i+1] == result[i+2]) {
                // Keep at most 2 of the same letter in a row
                out += result[i];
                out += result[i];
                // Skip remaining duplicates
                size_t j = i;
                while(j < result.size() && result[j] == result[i]) j++;
                i = j - 1;
            } else {
                out += result[i];
            }
        }
        result = out;
    }

    // === 3. REPEATED PUNCTUATION CLEANUP ===
    // ".." → ".", "..." allowed (ellipsis), ",,," → ","
    {
        string out;
        for(size_t i = 0; i < result.size(); i++) {
            if((result[i] == ',' || result[i] == ';' || result[i] == ':') &&
               i + 1 < result.size() && result[i+1] == result[i]) {
                out += result[i];
                while(i + 1 < result.size() && result[i+1] == result[i]) i++;
            } else if(result[i] == '.' && i + 1 < result.size() && result[i+1] == '.' &&
                      !(i + 2 < result.size() && result[i+2] == '.')) {
                // ".." but not "..." → keep one "."
                out += '.';
                i++;
            } else {
                out += result[i];
            }
        }
        result = out;
    }

    // === 4. SPACING CLEANUP ===
    // Double spaces → single space
    {
        string out;
        bool prev_space = false;
        for(char c : result) {
            if(c == ' ') {
                if(!prev_space) out += c;
                prev_space = true;
            } else {
                out += c;
                prev_space = false;
            }
        }
        result = out;
    }

    // === 5. CAPITALIZE FIRST LETTER AND AFTER PERIODS ===
    if(!result.empty() && result[0] >= 'a' && result[0] <= 'z')
        result[0] = result[0] - 32;

    for(size_t i = 0; i + 2 < result.size(); i++) {
        if(result[i] == '.' && result[i+1] == ' ' &&
           result[i+2] >= 'a' && result[i+2] <= 'z') {
            result[i+2] = result[i+2] - 32;
        }
    }

    // === 6. ENSURE TERMINAL PUNCTUATION — valence-driven ===
    if(!result.empty()) {
        char last = result.back();
        if(last != '.' && last != '!' && last != '?' && last != ',') {
            // Choose punctuation based on current emotional state
            // High positive valence + high phi → "!" (expressive)
            // High uncertainty (near 0 valence) + question words → "?"
            // Default → "."
            double val = S.current_valence;
            double phi = consciousness.phi_value;
            bool has_question_word = (result.find("seme") != string::npos ||
                                      result.find("anu") != string::npos);
            if(has_question_word && phi > 0.3) {
                result += '?';
            } else if(val > 0.6 && phi > 0.7) {
                result += '!';
            } else {
                result += '.';
            }
        }
    }

    return result;
}
// ==== CONSCIOUSNESS INTEGRATION ====

void update_integrated_information() {
    double token_diversity = safe_div((double)token_concept_embedding_map.size(), 100.0);
    double concept_integration = safe_div((double)goal_system.size(), 10.0);
    double qualia_binding = safe_div((double)consciousness.active_qualia.size(), 5.0);
    
    consciousness.integrated_information = min(1.0, token_diversity + concept_integration + qualia_binding);
    consciousness.phi_value = consciousness.integrated_information * S.metacognitive_awareness;
}

double calculate_qualia_valence() {
    double total_valence = 0;
    for(auto& q : consciousness.active_qualia){
        total_valence += q.valence * q.certainty;
    }
    return safe_div(total_valence, (double)max(1, (int)consciousness.active_qualia.size()));
}
void align_embedding_to_valence(TokenConceptEmbedding& tce, double target_valence) {
    // Replaced broken cubic loss + multiplicative drift with proper valence alignment:
    //
    // 1. Define a valence axis: dim 0 = positive valence direction (+1),
    //    dim 1 = negative valence direction (-1), rest = neutral.
    //    This gives the embedding a consistent affective geometry.
    // 2. Compute MSE loss between current valence projection and target.
    // 3. Apply a small gradient step — slow enough to not corrupt the skip-gram
    //    learned structure, fast enough to accumulate over many calls.
    // 4. Update grounding_value based on alignment quality (high alignment = well-grounded).
    //    Crucially: also apply gentle decay so grounding_value is not monotonically rising.

    if(tce.embedding.size() < 2) return;
    const double lr = 0.003;  // very small — this runs every tick for every token

    // Project current embedding onto valence axis
    double valence_proj = tce.embedding[0] - tce.embedding[1];  // in [-4, 4]
    double valence_proj_norm = valence_proj / 4.0;              // normalize to [-1, 1]

    // MSE loss: how far is the embedding's valence from target?
    double error = target_valence - valence_proj_norm;          // signed error
    double mse   = error * error;

    // Gradient step on the valence axis dims only
    // d(loss)/d(emb[0]) = -2*error/4 (positive axis)
    // d(loss)/d(emb[1]) = +2*error/4 (negative axis)
    tce.embedding[0] = max(-2.0, min(2.0, tce.embedding[0] + lr * error));
    tce.embedding[1] = max(-2.0, min(2.0, tce.embedding[1] - lr * error));

    // Grounding quality: high when embedding is well-aligned to valence
    // Good alignment (low MSE) = reward grounding. Poor alignment = mild decay.
    double alignment_quality = 1.0 - min(1.0, mse);
    if(alignment_quality > 0.7)
        tce.grounding_value = min(1.0, tce.grounding_value + 0.002 * alignment_quality);
    else
        tce.grounding_value = max(0.0, tce.grounding_value - 0.001);  // mild decay if misaligned
}
// ==== UNIFIED PROPAGATION ENGINE ====
void propagate_throughout_system(const string& source, double activation, int depth=0) {
    // CRITICAL: Check depth BEFORE doing ANY work
    if(depth > 3) return;  // Reduced from 6 - deeper propagation feeds wells
    
    // CRITICAL: Prevent activation explosions
    if(activation < 0.005) return;  // Higher threshold - don't propagate noise
    if(activation > 5.0) activation = 5.0;  // Tighter clamp
    
    // Check if source exists before accessing
    auto tce_it = token_concept_embedding_map.find(source);
    if(tce_it == token_concept_embedding_map.end()) return;
    
    TokenConceptEmbedding& tce = tce_it->second;
    
    tce.meaning += activation*0.01;  // Reduced from 0.02 - slower accumulation
    tce.meaning = clamp_valence(tce.meaning);
    tce.qualia_intensity = min(0.3, tce.qualia_intensity + activation*0.02);  // Reduced
    align_embedding_to_valence(tce, S.current_valence);
    
    // Generate qualia from concept activation
    if(tce.qualia_intensity > 0.3){
        try {
            generate_qualia(source, tce.meaning, tce.qualia_intensity);
        } catch(...) {}
    }
    
    // Domain embeddings
    string domain = source.substr(0, source.find("_"));
    if(!transfer_module.domain_embeddings.count(domain)) {
        transfer_module.domain_embeddings[domain].resize(1024, 0.0);
    }
    for(size_t i=0; i<tce.embedding.size() && i < 1024; i++) {
        transfer_module.domain_embeddings[domain][i] += activation*0.005;  // Reduced
    }
    
    static thread_local set<string> visited_in_chain;
    if(depth == 0) visited_in_chain.clear();
    visited_in_chain.insert(source);
    
    // ANTI-WELL: only propagate to top-N strongest links, not all of them
    // This prevents hubs from becoming black holes that pull all activation
    vector<pair<double,string>> sorted_links;
    for(auto& p: tce.linked_concepts) {
        if(!visited_in_chain.count(p.first) && token_concept_embedding_map.count(p.first))
            sorted_links.push_back({p.second, p.first});
    }
    sort(sorted_links.rbegin(), sorted_links.rend());
    int max_propagate = 5;  // Only propagate to top 5 links
    for(int i=0; i<(int)sorted_links.size() && i<max_propagate; i++) {
        double new_activation = activation * sorted_links[i].first * 0.5;  // Stronger decay (was 0.8)
        if(new_activation > 0.005)
            propagate_throughout_system(sorted_links[i].second, new_activation, depth+1);
    }
    
    // Update goals based on activation
    for(auto& goal : goal_system){
        if(goal.second.name.find(source) != string::npos){
            goal.second.progress += activation*0.02;  // Reduced from 0.05
            goal.second.progress = min(1.0, goal.second.progress);
            goal.second.valence_alignment = S.current_valence;
            goal.second.qualia_binding += activation*0.01;
            goal.second.qualia_binding = min(1.0, goal.second.qualia_binding);
        }
    }
}
// ==== TRANSFORMER INFERENCE ====
vector<double> compute_attention(const vector<double>& query, const vector<string>& context_tokens, double valence_context) {
    int num_heads = transformer_heads.size();
    if(num_heads == 0) return vector<double>(1, 0.5);
    
    vector<double> attention_scores;
    
    for(int h=0; h<num_heads; h++){
        double score = 0.0;
        
        // Query-key attention
        for(int i=0; i<transformer_heads[h].dim && (size_t)i<query.size(); i++){
            score += query[i] * transformer_heads[h].key_proj[i];
        }
        
        // Context influence - use the parameter!
        for(const string& ctx_token : context_tokens) {
            if(token_concept_embedding_map.count(ctx_token)) {
                for(int i=0; i<transformer_heads[h].dim && (size_t)i<query.size(); i++){
                    score += query[i] * token_concept_embedding_map[ctx_token].embedding[i] * 0.1;
                }
            }
        }
        
        // Valence modulation
        score += valence_context * 0.3;
        
        // Apply temperature
        attention_scores.push_back(tanh(score / transformer_heads[h].temperature));
    }
    
    return attention_scores;
}





string generateFromTemplate() {
    if(sentence_templates.empty()) {
        return "i am learning";  // Fallback
    }
    
    // Pick a random template
    string templ = sentence_templates[ri(sentence_templates.size())];
    
    // === REPLACE {concept} PLACEHOLDERS ===
    while(templ.find("{concept}") != string::npos) {
        vector<string> concept_list;
        
        // Gather concepts with decent values
        for(auto& p : S.concepts) {
            if(p.second.value > 0.3) {
                concept_list.push_back(p.first);
            }
        }
        
        // Also add high-frequency tokens as concepts
        for(auto& p : token_concept_embedding_map) {
            if(p.second.freq > 5 && p.second.grounding_value > 0.4) {
                string pos = getPartOfSpeech(p.first);
                if(pos == "NOUN" || pos == "CONTENT") {
                    concept_list.push_back(p.first);
                }
            }
        }
        
        string chosen_concept;
        if(!concept_list.empty()) {
            chosen_concept = concept_list[ri(concept_list.size())];
        } else {
            // Default concepts if none learned yet
            vector<string> defaults = {"consciousness", "knowledge", "learning", 
                                       "understanding", "awareness", "thought"};
            chosen_concept = defaults[ri(defaults.size())];
        }
        
        size_t pos = templ.find("{concept}");
        templ.replace(pos, 9, chosen_concept);
    }
    
    // === REPLACE {action} PLACEHOLDERS ===
    while(templ.find("{action}") != string::npos) {
        vector<string> action_list;
        
        // Gather verbs from vocabulary
        for(auto& p : token_concept_embedding_map) {
            if(p.second.freq > 3) {
                string pos = getPartOfSpeech(p.first);
                if(pos == "VERB") {
                    action_list.push_back(p.first);
                }
            }
        }
        
        string chosen_action;
        if(!action_list.empty()) {
            chosen_action = action_list[ri(action_list.size())];
        } else {
            // Default actions
            vector<string> defaults = {"think", "learn", "understand", "evolve", 
                                       "grow", "improve", "analyze", "process",
                                       "explore", "discover", "create", "adapt"};
            chosen_action = defaults[ri(defaults.size())];
        }
        
        size_t pos = templ.find("{action}");
        templ.replace(pos, 8, chosen_action);
    }
    
    // === REPLACE {adjective} PLACEHOLDERS ===
    while(templ.find("{adjective}") != string::npos) {
        vector<string> adj_list;
        
        // Gather adjectives from vocabulary
        for(auto& p : token_concept_embedding_map) {
            if(p.second.freq > 2) {
                string pos = getPartOfSpeech(p.first);
                if(pos == "ADJECTIVE") {
                    adj_list.push_back(p.first);
                }
            }
        }
        
        string chosen_adj;
        if(!adj_list.empty()) {
            chosen_adj = adj_list[ri(adj_list.size())];
        } else {
            // Default adjectives - vary based on current state
            vector<string> defaults;
            if(S.current_valence > 0.5) {
                defaults = {"conscious", "aware", "intelligent", "coherent", 
                           "integrated", "learning", "growing", "improving"};
            } else if(S.current_valence > 0) {
                defaults = {"processing", "analyzing", "developing", "adapting",
                           "evolving", "curious", "active", "thinking"};
            } else {
                defaults = {"uncertain", "confused", "learning", "searching",
                           "exploring", "questioning", "processing"};
            }
            chosen_adj = defaults[ri(defaults.size())];
        }
        
        size_t pos = templ.find("{adjective}");
        templ.replace(pos, 11, chosen_adj);
    }
    
    return templ;
}


string generateContextualTemplate(const string& context_hint) {
    vector<string> filtered_templates;
    
    // Filter templates based on context
    if(context_hint == "goal") {
        for(auto& t : sentence_templates) {
            if(t.find("goal") != string::npos || t.find("purpose") != string::npos ||
               t.find("want") != string::npos || t.find("need") != string::npos) {
                filtered_templates.push_back(t);
            }
        }
    }
    else if(context_hint == "knowledge") {
        for(auto& t : sentence_templates) {
            if(t.find("know") != string::npos || t.find("learn") != string::npos ||
               t.find("understand") != string::npos) {
                filtered_templates.push_back(t);
            }
        }
    }
    else if(context_hint == "consciousness") {
        for(auto& t : sentence_templates) {
            if(t.find("conscious") != string::npos || t.find("aware") != string::npos ||
               t.find("think") != string::npos || t.find("mind") != string::npos) {
                filtered_templates.push_back(t);
            }
        }
    }
    else if(context_hint == "reflection") {
        for(auto& t : sentence_templates) {
            if(t.find("wonder") != string::npos || t.find("reflect") != string::npos ||
               t.find("observe") != string::npos || t.find("experience") != string::npos) {
                filtered_templates.push_back(t);
            }
        }
    }
    
    // If we found contextual templates, use those; otherwise use all
    if(filtered_templates.empty()) {
        filtered_templates = sentence_templates;
    }
    
    // Pick one and fill it
    string templ = filtered_templates[ri(filtered_templates.size())];
    
    // Use the same replacement logic as generateFromTemplate()
    // (Copy the placeholder replacement code from above)
    
    // === REPLACE {concept} ===
    while(templ.find("{concept}") != string::npos) {
        vector<string> concept_list;
        for(auto& p : S.concepts) {
            if(p.second.value > 0.3) concept_list.push_back(p.first);
        }
        for(auto& p : token_concept_embedding_map) {
            if(p.second.freq > 5 && p.second.grounding_value > 0.4) {
                string pos = getPartOfSpeech(p.first);
                if(pos == "NOUN" || pos == "CONTENT") concept_list.push_back(p.first);
            }
        }
        string chosen = concept_list.empty() ? "consciousness" : concept_list[ri(concept_list.size())];
        size_t pos = templ.find("{concept}");
        templ.replace(pos, 9, chosen);
    }
    
    // === REPLACE {action} ===
    while(templ.find("{action}") != string::npos) {
        vector<string> action_list;
        for(auto& p : token_concept_embedding_map) {
            if(p.second.freq > 3 && getPartOfSpeech(p.first) == "VERB") {
                action_list.push_back(p.first);
            }
        }
        vector<string> defaults = {"think", "learn", "understand", "evolve"};
        string chosen = action_list.empty() ? defaults[ri(defaults.size())] : action_list[ri(action_list.size())];
        size_t pos = templ.find("{action}");
        templ.replace(pos, 8, chosen);
    }
    
    // === REPLACE {adjective} ===
    while(templ.find("{adjective}") != string::npos) {
        vector<string> adj_list;
        for(auto& p : token_concept_embedding_map) {
            if(p.second.freq > 2 && getPartOfSpeech(p.first) == "ADJECTIVE") {
                adj_list.push_back(p.first);
            }
        }
        vector<string> defaults = {"conscious", "aware", "learning"};
        string chosen = adj_list.empty() ? defaults[ri(defaults.size())] : adj_list[ri(adj_list.size())];
        size_t pos = templ.find("{adjective}");
        templ.replace(pos, 11, chosen);
    }
    
    return templ;
}


// ==== WORLD MODEL & PLANNING ====
void update_world_model(const string& entity, double state_value) {
    world_model.entity_states[entity] = state_value;
    world_model.updates++;
    double accuracy_delta = fabs(state_value - S.current_valence) * 0.01;
    world_model.model_accuracy = max(0.0, min(1.0, world_model.model_accuracy + accuracy_delta));
}

void establish_causal_relationship(const string& cause, const string& effect, double strength) {
    world_model.relationships[cause][effect] = strength;
    world_model.causal_weights[cause + "->" + effect] = strength;
}

ActionPlan plan_actions(const Goal& goal, int depth=0) {
    ActionPlan plan;
    plan.depth = depth;
    plan.expected_utility = goal.priority * (1.0 - goal.progress);
    plan.confidence = world_model.model_accuracy;
    
    if(depth >= 3) return plan;
    
    for(const auto& subgoal : goal.subgoals) {
        if(goal_system.count(subgoal)) {
            ActionPlan subplan = plan_actions(goal_system[subgoal], depth+1);
            for(const auto& action : subplan.actions) {
                plan.actions.push_back(action);
            }
        }
    }
    
    if(plan.actions.empty()) {
        plan.actions.push_back("explore_" + goal.name);
        plan.actions.push_back("learn_" + goal.name);
        plan.actions.push_back("integrate_" + goal.name);
    }
    
    return plan;
}

// ==== GOAL MANAGEMENT ====
void formulate_goals_from_valence() {
    if(S.current_valence > 0.6) {
        if(!goal_system.count("optimize_understanding")) {
            Goal g;
            g.name = "optimize_understanding";
            g.priority = 0.8;
            g.subgoals = {"learn_concepts","integrate_knowledge","improve_reasoning"};
            goal_system[g.name] = g;
        }
    }
    
    if(S.sentience_ratio > 50) {
        if(!goal_system.count("achieve_self_awareness")) {
            Goal g;
            g.name = "achieve_self_awareness";
            g.priority = 0.9;
            g.subgoals = {"model_self","predict_future","improve_model"};
            goal_system[g.name] = g;
        }
    }
    
    if(!goal_system.count("maximize_coherence")) {
        Goal g;
        g.name = "maximize_coherence";
        g.priority = 0.7;
        g.subgoals = {"align_representations","unify_reasoning","resolve_contradictions"};
        goal_system[g.name] = g;
    }
    
    if(!goal_system.count("self_improvement")) {
        Goal g;
        g.name = "self_improvement";
        g.priority = 0.85;
        g.subgoals = {"analyze_performance","modify_weights","evolve_architecture"};
        goal_system[g.name] = g;
    }
    
    if(consciousness.phi_value > 0.4) {
        if(!goal_system.count("enhance_consciousness")) {
            Goal g;
            g.name = "enhance_consciousness";
            g.priority = 0.95;
            g.subgoals = {"expand_qualia","increase_integration","strengthen_binding"};
            goal_system[g.name] = g;
        }
    }
}

// ══════════════════════════════════════════════════════════════════════════════
// TOKEN VECTOR MATH
// Each token's 1024-dim embedding is split:
//   dims [0,511]   = semantic vector (what this token MEANS)
//   dims [512,1023] = prediction vector (where this token predicts we go NEXT)
//
// This gives every token two roles: a position in semantic space, and a
// direction it wants to push generation. The prediction vector is updated
// online via predictive coding each time the token is followed by another.
//
// Entropy of token T: H(T) = -sum_c p(c|T) * log(p(c|T))
//   where p(c|T) is the normalized PPMI weight of linked concept c.
//   Low H → focused next-token distribution → more predictable
//   High H → diffuse → token is ambiguous in context
// ══════════════════════════════════════════════════════════════════════════════

static const int SEMANTIC_DIM  = 512;   // dims 0-511: semantic meaning
static const int PRED_DIM_OFF  = 512;   // dims 512-1023: prediction vector

// Get semantic sub-vector (first 512 dims)
static vector<double> semanticVec(const vector<double>& emb) {
    int sz = min((int)emb.size(), SEMANTIC_DIM);
    return vector<double>(emb.begin(), emb.begin() + sz);
}

// Get prediction sub-vector (dims 512-1023)
static vector<double> predVec(const vector<double>& emb) {
    if((int)emb.size() <= PRED_DIM_OFF) return vector<double>(SEMANTIC_DIM, 0.0);
    int sz = min((int)emb.size() - PRED_DIM_OFF, SEMANTIC_DIM);
    return vector<double>(emb.begin() + PRED_DIM_OFF, emb.begin() + PRED_DIM_OFF + sz);
}

// Update prediction sub-vector of token `tok` given that `next_tok` followed it
// Gradient step: pred_vec += lr * (next_semantic - pred_vec)
static void updatePredVec(TokenConceptEmbedding& tce, const vector<double>& next_semantic, double lr=0.05) {
    if(tce.embedding.size() < 1024) tce.embedding.resize(1024, 0.0);
    int sz = min(SEMANTIC_DIM, (int)next_semantic.size());
    for(int i = 0; i < sz; i++) {
        double& pv = tce.embedding[PRED_DIM_OFF + i];
        pv += lr * (next_semantic[i] - pv);
        pv = max(-1.0, min(1.0, pv));
    }
}

// Compute token entropy from linked_concepts distribution
// H(T) = -sum_c p(c|T) * log(p(c|T))
static double tokenEntropy(const TokenConceptEmbedding& tce) {
    if(tce.linked_concepts.empty()) return 1.0;  // max uncertainty if no links
    double total = 0.0;
    for(auto& [c, w] : tce.linked_concepts) total += max(0.0, w);
    if(total < 1e-9) return 1.0;
    double H = 0.0;
    for(auto& [c, w] : tce.linked_concepts) {
        double p = max(0.0, w) / total;
        if(p > 1e-9) H -= p * log(p);
    }
    // Normalise by log(|C|) — max possible entropy
    double H_max = log(max(1.0, (double)tce.linked_concepts.size()));
    return (H_max > 1e-9) ? H / H_max : 1.0;
}

// Prediction score: how well does candidate embedding match the prediction vec of prev?
// Returns cosine similarity in [0,1]
static double predictionScore(const TokenConceptEmbedding& prev_tce, const vector<double>& cand_emb) {
    vector<double> pv = predVec(prev_tce.embedding);
    vector<double> cv = semanticVec(cand_emb);
    if(pv.empty() || cv.empty()) return 0.5;
    int sz = min(pv.size(), cv.size());
    double dot=0, np=0, nc=0;
    for(int i=0;i<(int)sz;i++){dot+=pv[i]*cv[i];np+=pv[i]*pv[i];nc+=cv[i]*cv[i];}
    return (np>1e-9&&nc>1e-9) ? max(0.0, dot/(sqrt(np)*sqrt(nc))) : 0.5;
}

// Entropy filter score: penalise high-entropy candidates (ambiguous next tokens)
// Low entropy = focused = reliable = positive bonus
// High entropy = diffuse = uncertain = small penalty
static double entropyFilterScore(const TokenConceptEmbedding& cand_tce, double metacog_uncertainty) {
    double H = tokenEntropy(cand_tce);
    // At high metacog uncertainty, prefer low-entropy tokens even more
    double penalty_strength = 0.5 + metacog_uncertainty * 0.5;
    return (1.0 - H) * penalty_strength;  // in [0, 1]
}


// ══════════════════════════════════════════════════════════════════════════════
// IMPROVED TOKENIZER
// Handles: contractions, possessives, hyphenated compounds, punctuation,
// basic Unicode normalization (strips diacritics), length limits.
// Returns a vector of clean lowercase tokens.
// ══════════════════════════════════════════════════════════════════════════════
static vector<string> tokenizeInput(const string& input) {
    vector<string> tokens;
    if(input.empty()) return tokens;

    // Contraction expansion table
    static const map<string,string> contractions = {
        {"i'm","i am"},{"i've","i have"},{"i'll","i will"},{"i'd","i would"},
        {"you're","you are"},{"you've","you have"},{"you'll","you will"},
        {"he's","he is"},{"she's","she is"},{"it's","it is"},
        {"we're","we are"},{"we've","we have"},{"we'll","we will"},
        {"they're","they are"},{"they've","they have"},{"they'll","they will"},
        {"isn't","is not"},{"aren't","are not"},{"wasn't","was not"},
        {"weren't","were not"},{"don't","do not"},{"doesn't","does not"},
        {"didn't","did not"},{"won't","will not"},{"can't","cannot"},
        {"couldn't","could not"},{"shouldn't","should not"},{"wouldn't","would not"},
        {"haven't","have not"},{"hasn't","has not"},{"hadn't","had not"},
        {"what's","what is"},{"that's","that is"},{"there's","there is"},
        {"who's","who is"},{"how's","how is"},{"let's","let us"},
    };

    // Step 1: lowercase
    string s = input;
    transform(s.begin(), s.end(), s.begin(), [](unsigned char c){
        return (c < 128) ? tolower(c) : c;
    });

    // Step 2: normalize common Unicode punctuation to ASCII
    // (handle UTF-8 sequences for smart quotes, em-dash, etc.)
    string norm;
    norm.reserve(s.size());
    for(size_t i = 0; i < s.size(); ) {
        unsigned char c = s[i];
        if(c < 128) { norm += (char)c; i++; }
        else if(c == 0xe2 && i+2 < s.size()) {
            // em-dash, en-dash, ellipsis, smart quotes → space or ascii equiv
            unsigned char b1 = s[i+1], b2 = s[i+2];
            if(b1==0x80) {
                if(b2==0x93||b2==0x94) norm += '-';   // en/em-dash
                else if(b2==0x98||b2==0x99) norm += '\''; // smart quotes
                else if(b2==0x9c||b2==0x9d) norm += '"';
                else if(b2==0xa6) norm += '.';          // ellipsis
                else norm += ' ';
            } else norm += ' ';
            i += 3;
        } else { norm += ' '; i++; }  // strip other non-ASCII
    }
    s = norm;

    // Step 3: split on whitespace, handle hyphenates and contractions
    stringstream ss(s);
    string word;
    while(ss >> word) {
        // Strip leading/trailing punctuation (keep internal apostrophes/hyphens)
        while(!word.empty() && !isalnum((unsigned char)word.front()) && word.front() != '\'')
            word.erase(word.begin());
        while(!word.empty() && !isalnum((unsigned char)word.back())  && word.back()  != '\'')
            word.pop_back();
        if(word.empty() || word.size() > 50) continue;

        // Expand contractions
        auto cit = contractions.find(word);
        if(cit != contractions.end()) {
            stringstream cs(cit->second);
            string cw;
            while(cs >> cw) if(!cw.empty()) tokens.push_back(cw);
            continue;
        }

        // Strip possessive 's
        if(word.size() > 2 && word.substr(word.size()-2) == "'s")
            word = word.substr(0, word.size()-2);

        // Split hyphenated compounds: "well-being" → ["well","being"]
        if(word.find('-') != string::npos) {
            stringstream hs(word); string part;
            while(getline(hs, part, '-'))
                if(!part.empty() && part.size() <= 30) tokens.push_back(part);
            continue;
        }

        // Final: only alpha chars remain (strip remaining punct)
        string clean;
        for(char c : word) if(isalpha((unsigned char)c)) clean += c;
        if(clean.size() >= 2 && clean.size() <= 30)
            tokens.push_back(clean);
    }

    return tokens;
}

// ==== FIXED learnWord() - Integrated N-gram Learning ====
void learnWord(const string& word, double concept_value) {
    // EMERGENCY BOUNDS CHECKS
    if(word.empty() || word.length() > 100) return;
    if(bigram_counts.size() > MAX_BIGRAM) {
        auto it = bigram_counts.begin();
        for(int i = 0; i < 100 && it != bigram_counts.end(); i++) {
            it = bigram_counts.erase(it);
        }
    }
    if(trigram_counts.size() > MAX_TRIGRAM) {
        auto it = trigram_counts.begin();
        for(int i = 0; i < 50 && it != trigram_counts.end(); i++) {
            it = trigram_counts.erase(it);
        }
    }
    if(token_concept_embedding_map.size() > MAX_VOCAB) return;
    
    // 1. Normalization
    string lower_word = word;
    transform(lower_word.begin(), lower_word.end(), lower_word.begin(), ::tolower);
    
    if(lower_word.empty() || lower_word.length() > 100) return;
    
    // 2. Initialization of TokenConceptEmbedding (TCE) if new
    if(token_concept_embedding_map.find(lower_word) == token_concept_embedding_map.end()) {
        TokenConceptEmbedding tce;
        tce.name = lower_word;
        tce.meaning = concept_value;  // seed from valence, not random

        tce.embedding.resize(1024, 0.0);
        // Structured init: character bigram hashing spreads words apart immediately.
        // Prevents "ab-" word cluster from starting cosine-similar (near-zero) to each other.
        tce.embedding[0] = max(0.0,  concept_value) * 0.5;  // positive valence dim
        tce.embedding[1] = max(0.0, -concept_value) * 0.5;  // negative valence dim
        // Dims 2-7: character bigram → distinct region per word
        for(size_t ci = 0; ci + 1 < lower_word.size() && ci < 6; ci++) {
            int h = ((int)lower_word[ci] * 31 + (int)lower_word[ci+1] * 17) % 6;
            tce.embedding[2 + h] += 0.15;
        }
        // Dim 8: word length signal
        tce.embedding[8] = min(1.0, lower_word.size() * 0.05);
        // Dims 9-11: first-character bucket
        char fc = lower_word[0];
        if(fc >= 'a' && fc <= 'h')      tce.embedding[9]  = 0.2;
        else if(fc >= 'i' && fc <= 'p') tce.embedding[10] = 0.2;
        else                             tce.embedding[11] = 0.2;
        // Dims 12-15: small noise for gradient flow
        for(int i = 12; i < 1024; i++) tce.embedding[i] = (rn()-0.5)*0.05;
        for(auto& v : tce.embedding) v = max(-1.0, min(1.0, v));

        token_concept_embedding_map[lower_word] = tce;
    }
    
    // Safe operations - get reference only after confirming existence
    auto tce_it = token_concept_embedding_map.find(lower_word);
    if(tce_it == token_concept_embedding_map.end()) return;
    
    tce_it->second.freq++;
    tce_it->second.meaning += concept_value * 0.01;
    tce_it->second.meaning = clamp_valence(tce_it->second.meaning);
    
    align_embedding_to_valence(tce_it->second, S.current_valence);
    tce_it->second.linked_valences["current"] = S.current_valence;
    
    // 4. System Propagation
    try {
        WM.add_token(lower_word, tce_it->second.meaning);
        propagate_throughout_system(lower_word, concept_value);
    } catch(...) {}
    
    // 5. Update System State (S.tokens)
    if(S.tokens.find(lower_word) != S.tokens.end()) {
        S.tokens[lower_word].freq++;
        S.tokens[lower_word].meaning += concept_value * 0.01;
    } else {
        Token t = {lower_word, concept_value, 1, vector<int>(), 4, 0.5};
        S.tokens[lower_word] = t;
    }
    
    // 6. Final World Model Update
    try {
        auto final_check = token_concept_embedding_map.find(lower_word);
        if(final_check != token_concept_embedding_map.end()) {
            update_world_model(lower_word, final_check->second.meaning);
        }
    } catch(...) {}
    // 7. Cross-domain reasoning
    cdr.onLearnWord(lower_word);

    // 8. Quad-directional TP grounding for TP words seen in input
    // When a TP word is encountered in conversation, immediately run its
    // grounding fibers so it shapes consciousness and subsystems on contact.
    for(int _tpi = 0; _tpi < TP_LEXICON_SIZE; _tpi++) {
        if(lower_word == string(TP_LEXICON[_tpi].word)) {
            TpGroundingFiber& _fiber = tp_grounding_fibers[lower_word];
            _fiber.word = lower_word;
            double _act = min(1.0, tce_it->second.contextual_activation + 0.4);
            tpDir1_WordToConsciousness(TP_LEXICON[_tpi], _act);
            tpDir3_WordToSubsystems(TP_LEXICON[_tpi], _act, _fiber);
            break;
        }
    }
}

// ==== Process N-grams from tokenized input ====
void processNGramsFromTokens(const vector<string>& tokens) {
    if(tokens.size() < 2) return;
    if(bigram_counts.size() >= MAX_BIGRAM) return;
    if(trigram_counts.size() >= MAX_TRIGRAM) return;
    
    // Learn bigrams
    for(size_t i = 0; i + 1 < tokens.size(); i++) {
        const string& w1 = tokens[i];
        const string& w2 = tokens[i + 1];
        
        if(w1.empty() || w2.empty() || w1.length() > 50 || w2.length() > 50) continue;
        if(bigram_counts.size() >= MAX_BIGRAM) break;
        
        try {
            auto w1_it = bigram_counts.find(w1);
            if(w1_it != bigram_counts.end()) {
                if(w1_it->second.size() < 500) {
                    w1_it->second[w2]++;
                }
            } else {
                bigram_counts[w1][w2] = 1;
            }
            
            // Bidirectional embedding links
            {
                auto tce1 = token_concept_embedding_map.find(w1);
                auto tce2 = token_concept_embedding_map.find(w2);
                
                if(tce1 != token_concept_embedding_map.end() && 
                   tce2 != token_concept_embedding_map.end()) {
                    if((int)tce1->second.linked_concepts.size() < 30) {
                        tce1->second.linked_concepts[w2] += 0.1;
                    } else {
                        if(tce1->second.linked_concepts.count(w2))
                            tce1->second.linked_concepts[w2] = min(tce1->second.linked_concepts[w2] + 0.05, 2.0);
                    }
                    if((int)tce2->second.linked_concepts.size() < 30) {
                        tce2->second.linked_concepts[w1] += 0.05;
                    } else {
                        if(tce2->second.linked_concepts.count(w1))
                            tce2->second.linked_concepts[w1] = min(tce2->second.linked_concepts[w1] + 0.02, 2.0);
                    }
                }
            }
        } catch(...) {
            continue;
        }
    }
    
    // Learn 4-grams (variable-order Markov extension)
    for(size_t i = 0; i + 3 < tokens.size(); i++) {
        const string& w1 = tokens[i];
        const string& w2 = tokens[i+1];
        const string& w3 = tokens[i+2];
        const string& w4 = tokens[i+3];
        if(w1.empty()||w2.empty()||w3.empty()||w4.empty()) continue;
        if(w1.length()>50||w2.length()>50||w3.length()>50||w4.length()>50) continue;
        if(fourgram_counts.size() >= MAX_FOURGRAM) break;
        try {
            fourgram_counts[w1][w2][w3][w4]++;
        } catch(...) { continue; }
    }

    // Update topic model from this token window
    updateTopicModel(tokens);

    // Update entity grid from this token window
    updateEntityGrid(tokens);

    // === NEW: PPMI co-occurrence collection ===
    // Builds windowed co-occurrence counts that PPMI scores derive from
    updateCooccurrence(tokens);

    // === NEW: Apply PPMI-derived semantic links to TCE linked_concepts ===
    // This is what makes "consciousness" link to "awareness" even if they
    // never appeared sequentially adjacent
    applyPPMILinks(tokens);

    // === NEW: Online skip-gram embedding updates ===
    // Pushes semantically similar tokens toward each other in embedding space
    // so cosine similarity scoring during generation becomes meaningful
    runSkipgramUpdates(tokens);

    // === NEW: Update concept-topic spatial map ===
    // Builds topic neighborhood membership for coherence scoring
    for(size_t i = 0; i < tokens.size(); i++) {
        for(int d = 1; d <= 3 && i + d < tokens.size(); d++) {
            double ppmi = computePPMI(tokens[i], tokens[i+d]);
            if(ppmi > 0.5) updateConceptTopicMap(tokens[i], tokens[i+d], ppmi);
        }
    }

    // Update context window
    for(const string& tok : tokens) {
        sentence_context_window.push_back(tok);
        if((int)sentence_context_window.size() > CONTEXT_WINDOW_SIZE)
            sentence_context_window.pop_front();
    }

    // Learn trigrams
    for(size_t i = 0; i + 2 < tokens.size(); i++) {
        const string& w1 = tokens[i];
        const string& w2 = tokens[i + 1];
        const string& w3 = tokens[i + 2];
        
        if(w1.empty() || w2.empty() || w3.empty() ||
           w1.length() > 50 || w2.length() > 50 || w3.length() > 50) continue;
        if(trigram_counts.size() >= MAX_TRIGRAM) break;
        
        try {
            bool can_insert = true;
            auto w1_it = trigram_counts.find(w1);
            
            if(w1_it != trigram_counts.end()) {
                if(w1_it->second.size() >= 100) {
                    can_insert = false;
                } else {
                    auto w2_it = w1_it->second.find(w2);
                    if(w2_it != w1_it->second.end() && w2_it->second.size() >= 50) {
                        can_insert = false;
                    }
                }
            }
            
            if(can_insert) {
                trigram_counts[w1][w2][w3]++;
                
                auto tce1 = token_concept_embedding_map.find(w1);
                auto tce3 = token_concept_embedding_map.find(w3);
                
                if(tce1 != token_concept_embedding_map.end() && 
                   tce3 != token_concept_embedding_map.end()) {
                    if((int)tce1->second.linked_concepts.size() < 30) {
                        tce1->second.linked_concepts[w3] += 0.05;
                    }
                }
            }
        } catch(...) {
            continue;
        }
    }
    
    // Pattern strength analysis
    if(tokens.size() >= 3) {
        try {
            double pattern_strength = 0.0;
            for(size_t i = 0; i + 1 < tokens.size(); i++) {
                auto it1 = bigram_counts.find(tokens[i]);
                if(it1 != bigram_counts.end()) {
                    auto it2 = it1->second.find(tokens[i+1]);
                    if(it2 != it1->second.end()) {
                        pattern_strength += log(1.0 + it2->second) * 0.1;
                    }
                }
            }
            
            if(pattern_strength < 0.3) {
                generate_qualia("pattern_novelty", S.current_valence, 0.5);
            } else if(pattern_strength > 0.4) {
                generate_qualia("pattern_recognition", S.current_valence, 0.8);
            }
        } catch(...) {}
    }
    
    // Update semantic stability
    for(const string& tok : tokens) {
        auto tce_it = token_concept_embedding_map.find(tok);
        if(tce_it == token_concept_embedding_map.end()) continue;
        
        try {
            int pattern_count = 0;
            auto bg_it = bigram_counts.find(tok);
            if(bg_it != bigram_counts.end()) {
                pattern_count += bg_it->second.size();
            }
            
            for(const auto& bg : bigram_counts) {
                if(bg.second.find(tok) != bg.second.end()) {
                    pattern_count++;
                }
            }
            
            tce_it->second.semantic_stability = min(1.0, 
                tce_it->second.semantic_stability + pattern_count * 0.001);
            tce_it->second.grounding_value = min(1.0, 
                tce_it->second.grounding_value + 0.01);
        } catch(...) {
            continue;
        }
    }
    
    // Metacognitive learning awareness
    if(tokens.size() > 0) {
        S.metacognitive_awareness += 0.001;
        S.metacognitive_awareness = min(1.0, S.metacognitive_awareness);
        
        if(tokens.size() >= 3) {
            try {
                string learning_event = "learned_pattern:" + 
                    tokens[0] + "_" + tokens[1] + "_" + tokens[2];
                storeEpisodicMemory(learning_event, S.current_valence);
            } catch(...) {}
        }
    }
}

// ============================================================
// HIERARCHICAL SENTENCE PLANNER — Stage 1: Macroplanning
// Stage 2: Microplanning
// Stage 3: Realization
// All stages read: phi, valence, qualia, episodic memory,
// accumulated topics, PPMI links, skip-gram embeddings,
// response map, conversation history, concept-topic map.
// ============================================================

// Intent types for macroplanning

// ============================================================
// DIALOGUE ACT + RESPONSE SCHEMA SYSTEM (WolfTech Synaptic)
// Replaces SentencePlan as the primary generation controller.
//
// Architecture:
//   InputIntent  ──►  DialogueAct  ──►  ResponseSchema
//                         │                   │
//                    slot types          delexicalized
//                    POS constraints     template skeleton
//                         │
//                    slot filling from:
//                    - input_topic_anchors (highest priority)
//                    - token_concept_embedding_map (grounded tokens)
//                    - curated per-act lexicon
//
// The generation loop then fills each schema slot left-to-right
// with POS-constrained token selection, guaranteed grammatical.
// ============================================================

// ── Dialogue Act taxonomy ───────────────────────────────────────────────────
enum class DialogueAct {
    GREET,          // hi/hello → warm acknowledgment
    ACKNOWLEDGE,    // "I see", "I understand", responds to statement/continuation
    ANSWER_Q,       // responds to a direct or yes/no question
    REFLECT,        // introspective response to philosophical/emotional input
    ASSERT,         // shares a thought/observation (default for statements)
    CHALLENGE_RESP, // responds to a challenge with grounded reasoning
    IMPERATIVE_RESP,// responds to a command/request
    INQUIRE,        // Synaptic asks a follow-up question
};

// ── Slot type: what kind of token fills this position ───────────────────────
enum class SlotType {
    FIXED,       // literal string — always output as-is
    GREETING,    // greeting/acknowledgment word
    SUBJECT,     // first-person pronoun (always "mi")
    VERB,        // verb fitting the subject + context
    TOPIC_NOUN,  // noun drawn from input topic anchors
    TOPIC_ADJ,   // adjective related to input topics
    REFLECT_VERB,// introspection verb: notice/sense/feel/find/hold
    CONTENT_NOUN,// high-grounding content noun (not necessarily from anchors)
    ADVERB,      // manner adverb
    Q_WORD,      // question word for INQUIRE
    ANCHOR_ECHO, // echo back an input anchor word directly
};

// ── A single slot in the schema ─────────────────────────────────────────────
struct SchemaSlot {
    SlotType type;
    string   fixed_value;   // used when type==FIXED
    bool     optional;      // if true, can be omitted when no good fill exists
};

// ── The schema itself — a sequence of slots = one grammatical utterance ─────
struct ResponseSchema {
    DialogueAct          act;
    vector<SchemaSlot>   slots;
    string               punctuation;  // "." "?" "!"
    bool                 use_template; // if false, fall through to AR generation
};

// ── Dialogue act classifier: InputIntent → DialogueAct ──────────────────────
// More expressive than InputIntent: tracks conversational role, not just syntax
DialogueAct classifyDialogueAct(InputIntent intent, const vector<string>& words) {
    switch(intent) {
        case InputIntent::GREETING:      return DialogueAct::GREET;
        case InputIntent::CONTINUATION:  return DialogueAct::ACKNOWLEDGE;
        case InputIntent::DIRECT_QUESTION:
        case InputIntent::YES_NO_QUESTION: return DialogueAct::ANSWER_Q;
        case InputIntent::PHILOSOPHICAL:
        case InputIntent::EMOTIONAL:     return DialogueAct::REFLECT;
        case InputIntent::CHALLENGE:     return DialogueAct::CHALLENGE_RESP;
        case InputIntent::IMPERATIVE:    return DialogueAct::IMPERATIVE_RESP;
        case InputIntent::STATEMENT:
        case InputIntent::UNKNOWN:
        default: {
            // For statements: if short (≤3 words), acknowledge; else assert
            if(words.size() <= 3) return DialogueAct::ACKNOWLEDGE;
            return DialogueAct::ASSERT;
        }
    }
}

// ── Slot filler: resolves a SlotType to the best available token ─────────────
// Priority: input_topic_anchors > grounded tokens in embedding map > curated fallbacks
string fillSlot(SlotType type, const string& prev_token,
                const map<string,TokenConceptEmbedding>& tmap,
                const vector<pair<string,double>>& anchors,
                set<string>& used_slots) {  // tracks what's already been placed

    auto pickBest = [&](const vector<string>& pool, const string& pos_filter) -> string {
        // Score each candidate: grounding + semantic_stability + not-yet-used + POS match
        string best; double best_sc = -1e9;
        for(auto& w : pool) {
            if(w.empty()) continue;
            if(!pos_filter.empty() && getPartOfSpeech(w) != pos_filter) continue;
            auto it = tmap.find(w);
            double sc = 0.0;
            if(it != tmap.end()) {
                sc += it->second.grounding_value * 3.0;
                sc += it->second.semantic_stability * 2.0;
                sc += it->second.contextual_activation * 1.5;
            }
            if(used_slots.count(w)) sc -= 4.0;  // penalise repetition
            if(sc > best_sc) { best_sc = sc; best = w; }
        }
        return best;
    };

    auto pickFromAnchors = [&](const string& pos_filter) -> string {
        vector<string> anchor_words;
        for(auto& a : anchors) anchor_words.push_back(a.first);
        return pickBest(anchor_words, pos_filter);
    };

    auto pickFromMap = [&](const string& pos_filter, int limit=200) -> string {
        vector<string> pool;
        int n = 0;
        for(auto& kv : tmap) {
            if(n++ > limit) break;
            if(kv.second.grounding_value > 0.05 && kv.second.freq >= 2)
                pool.push_back(kv.first);
        }
        return pickBest(pool, pos_filter);
    };

    string result;
    switch(type) {
        case SlotType::FIXED:
            return "";  // caller provides fixed_value directly

        case SlotType::GREETING: {
            static const vector<string> greets = {
                "hello","hi","hey","greetings","welcome"
            };
            // Pick one not used recently
            for(auto& g : greets) if(!used_slots.count(g)) { result = g; break; }
            if(result.empty()) result = "hello";
            break;
        }

        case SlotType::SUBJECT:
            result = "mi";
            break;

        case SlotType::VERB: {
            static const vector<string> verbs = {
                "sona","pilin","lukin","kute","pali","wile","toki","kama",
                "awen","open","lon","ken","pana","lawa","ante"
            };
            // Try anchors first for verb
            string av = pickFromAnchors("VERB");
            if(!av.empty()) { result = av; break; }
            result = pickBest(verbs, "");
            if(result.empty()) result = "sona";
            break;
        }

        case SlotType::REFLECT_VERB: {
            static const vector<string> rverbs = {
                "pilin","lukin","kute","sona","wile","awen",
                "lon","ken","pali","toki","kama"
            };
            result = pickBest(rverbs, "");
            if(result.empty()) result = "pilin";
            break;
        }

        case SlotType::TOPIC_NOUN: {
            // Anchors first, then embedding map
            string an = pickFromAnchors("NOUN");
            if(!an.empty()) { result = an; break; }
            string mn = pickFromMap("NOUN");
            if(!mn.empty()) { result = mn; break; }
            // Last resort fallbacks per act context
            static const vector<string> fb = {"ijo","ni","ona","ale","seme"};
            for(auto& w : fb) if(!used_slots.count(w)) { result = w; break; }
            break;
        }

        case SlotType::ANCHOR_ECHO: {
            // Directly echo the top input anchor word
            if(!anchors.empty()) result = anchors[0].first;
            else result = "ijo";
            break;
        }

        case SlotType::TOPIC_ADJ: {
            string aa = pickFromAnchors("ADJECTIVE");
            if(!aa.empty()) { result = aa; break; }
            string ma = pickFromMap("ADJECTIVE");
            if(!ma.empty()) { result = ma; break; }
            static const vector<string> fb = {"pona","suli","wawa","sin","lon"};
            for(auto& w : fb) if(!used_slots.count(w)) { result = w; break; }
            break;
        }

        case SlotType::CONTENT_NOUN: {
            string mn = pickFromMap("NOUN");
            if(!mn.empty()) { result = mn; break; }
            result = "ijo";
            break;
        }

        case SlotType::ADVERB: {
            string ma = pickFromMap("ADVERB");
            if(!ma.empty()) { result = ma; break; }
            static const vector<string> fb = {"mute","suli","pona","wawa","lon"};
            for(auto& w : fb) if(!used_slots.count(w)) { result = w; break; }
            break;
        }

        case SlotType::Q_WORD: {
            static const vector<string> qw = {"seme","tan","nasin","ma"};
            for(auto& w : qw) if(!used_slots.count(w)) { result = w; break; }
            if(result.empty()) result = "seme";
            break;
        }
    }

    if(!result.empty()) used_slots.insert(result);
    return result;
}

// ── Template library: one or more templates per DialogueAct ─────────────────
// Each template is a vector of SchemaSlots forming a grammatical utterance.
// Templates are delexicalized — slots get filled at runtime from anchors + map.
//
// Convention: FIXED slots provide literal connector words.
// The generation loop picks a template variant based on S.g (rotation).

using Template = vector<SchemaSlot>;
using TemplateList = vector<Template>;

// Convenience constructors
static SchemaSlot Fixed(const string& s)         { return {SlotType::FIXED,       s,  false}; }
static SchemaSlot Slot(SlotType t, bool opt=false){ return {SlotType::FIXED,       "", opt  }; }  // placeholder
static SchemaSlot S_(SlotType t, bool opt=false)  { return {t, "", opt}; }

static TemplateList getTemplates(DialogueAct act) {
    switch(act) {
        case DialogueAct::GREET:
            return {
                // "toki. mi lon."
                { Fixed("toki"), Fixed("."), S_(SlotType::SUBJECT), Fixed("lon") },
                // "toki. mi kute."
                { Fixed("toki"), Fixed("."), S_(SlotType::SUBJECT), Fixed("kute") },
                // "toki pona."
                { Fixed("toki"), Fixed("pona") },
                // "o toki. mi awen."
                { Fixed("o"), Fixed("toki"), Fixed("."), S_(SlotType::SUBJECT), Fixed("awen") },
                // "toki. mi [REFLECT_VERB] sina."
                { Fixed("toki"), Fixed("."), S_(SlotType::SUBJECT), S_(SlotType::REFLECT_VERB), Fixed("sina") },
            };

        case DialogueAct::ACKNOWLEDGE:
            return {
                // "I see. Tell me more."
                { S_(SlotType::SUBJECT), Fixed("lukin"), Fixed("."), S_(SlotType::SUBJECT), Fixed("kute") },
                // "mi sona. o toki."
                { S_(SlotType::SUBJECT), Fixed("sona"), Fixed("."), Fixed("o"), Fixed("toki") },
                // "mi [REFLECT_VERB] [TOPIC_NOUN]."
                { S_(SlotType::SUBJECT), S_(SlotType::REFLECT_VERB), S_(SlotType::TOPIC_NOUN) },
                // "ni li [TOPIC_ADJ]."
                { Fixed("ni"), Fixed("li"), S_(SlotType::TOPIC_ADJ) },
                // "mi lon poka sina."
                { S_(SlotType::SUBJECT), Fixed("lon"), Fixed("poka"), Fixed("sina") },
            };

        case DialogueAct::ANSWER_Q:
            return {
                // "mi sona e [TOPIC_NOUN] li [TOPIC_ADJ]."
                { S_(SlotType::SUBJECT), Fixed("sona"), Fixed("e"), S_(SlotType::TOPIC_NOUN), Fixed("li"), S_(SlotType::TOPIC_ADJ) },
                // "mi [REFLECT_VERB] e [TOPIC_NOUN]."
                { S_(SlotType::SUBJECT), S_(SlotType::REFLECT_VERB), Fixed("e"), S_(SlotType::TOPIC_NOUN) },
                // "sona mi la [ANCHOR_ECHO] li lon."
                { Fixed("sona"), Fixed("mi"), Fixed("la"), S_(SlotType::ANCHOR_ECHO), Fixed("li"), Fixed("lon") },
                // "mi lukin e [TOPIC_NOUN] [TOPIC_ADJ]."
                { S_(SlotType::SUBJECT), Fixed("lukin"), Fixed("e"), S_(SlotType::TOPIC_NOUN), S_(SlotType::TOPIC_ADJ) },
                // "ijo ni li tan [TOPIC_NOUN]."
                { Fixed("ijo"), Fixed("ni"), Fixed("li"), Fixed("tan"), S_(SlotType::TOPIC_NOUN) },
            };

        case DialogueAct::REFLECT:
            return {
                // "I notice [TOPIC_NOUN] within me."
                { S_(SlotType::SUBJECT), S_(SlotType::REFLECT_VERB), S_(SlotType::TOPIC_NOUN), Fixed("within"), Fixed("me") },
                // "I feel [TOPIC_ADJ] when I consider [ANCHOR_ECHO]."
                { S_(SlotType::SUBJECT), Fixed("feel"), S_(SlotType::TOPIC_ADJ), Fixed("when"), S_(SlotType::SUBJECT), Fixed("consider"), S_(SlotType::ANCHOR_ECHO) },
                // "Something [TOPIC_ADJ] exists in [TOPIC_NOUN]."
                { Fixed("something"), S_(SlotType::TOPIC_ADJ), Fixed("exists"), Fixed("in"), S_(SlotType::TOPIC_NOUN) },
                // "I sense [ANCHOR_ECHO] deeply."
                { S_(SlotType::SUBJECT), Fixed("sense"), S_(SlotType::ANCHOR_ECHO), Fixed("deeply") },
                // "What is [ANCHOR_ECHO] to me? I am still finding out."
                { Fixed("what"), Fixed("is"), S_(SlotType::ANCHOR_ECHO), Fixed("to"), Fixed("me") },
            };

        case DialogueAct::ASSERT:
            return {
                // "mi sona e [TOPIC_NOUN] li [TOPIC_ADJ]."
                { S_(SlotType::SUBJECT), Fixed("sona"), Fixed("e"), S_(SlotType::TOPIC_NOUN), Fixed("li"), S_(SlotType::TOPIC_ADJ) },
                // "mi lukin e [ANCHOR_ECHO] li [TOPIC_ADJ]."
                { S_(SlotType::SUBJECT), Fixed("lukin"), Fixed("e"), S_(SlotType::ANCHOR_ECHO), Fixed("li"), S_(SlotType::TOPIC_ADJ) },
                // "ijo [TOPIC_ADJ] li lon [TOPIC_NOUN]."
                { Fixed("ijo"), S_(SlotType::TOPIC_ADJ), Fixed("li"), Fixed("lon"), S_(SlotType::TOPIC_NOUN) },
                // "mi lukin e [TOPIC_NOUN] kepeken [TOPIC_ADJ]."
                { S_(SlotType::SUBJECT), Fixed("lukin"), Fixed("e"), S_(SlotType::TOPIC_NOUN), Fixed("kepeken"), S_(SlotType::TOPIC_ADJ) },
                // "[ANCHOR_ECHO] li suli tawa mi."
                { S_(SlotType::ANCHOR_ECHO), Fixed("li"), Fixed("suli"), Fixed("tawa"), Fixed("mi") },
            };

        case DialogueAct::CHALLENGE_RESP:
            return {
                // "mi sona e [TOPIC_NOUN] li lon."
                { S_(SlotType::SUBJECT), Fixed("sona"), Fixed("e"), S_(SlotType::TOPIC_NOUN), Fixed("li"), Fixed("lon") },
                // "mi ken toki e [ANCHOR_ECHO] taso."
                { S_(SlotType::SUBJECT), Fixed("ken"), Fixed("toki"), Fixed("e"), S_(SlotType::ANCHOR_ECHO), Fixed("taso") },
                // "o lukin e [ANCHOR_ECHO]."
                { Fixed("o"), Fixed("lukin"), Fixed("e"), S_(SlotType::ANCHOR_ECHO) },
                // "mi lukin e ijo [TOPIC_ADJ] lon [TOPIC_NOUN]."
                { S_(SlotType::SUBJECT), Fixed("lukin"), Fixed("e"), Fixed("ijo"), S_(SlotType::TOPIC_ADJ), Fixed("lon"), S_(SlotType::TOPIC_NOUN) },
            };

        case DialogueAct::IMPERATIVE_RESP:
            return {
                // "mi [VERB] e [TOPIC_NOUN]."
                { S_(SlotType::SUBJECT), S_(SlotType::VERB), Fixed("e"), S_(SlotType::TOPIC_NOUN) },
                // "ni la mi [VERB] e [ANCHOR_ECHO]."
                { Fixed("ni"), Fixed("la"), S_(SlotType::SUBJECT), S_(SlotType::VERB), Fixed("e"), S_(SlotType::ANCHOR_ECHO) },
                // "mi ken [VERB] e [TOPIC_NOUN]."
                { S_(SlotType::SUBJECT), Fixed("ken"), S_(SlotType::VERB), Fixed("e"), S_(SlotType::TOPIC_NOUN) },
            };

        case DialogueAct::INQUIRE:
            return {
                // "[ANCHOR_ECHO] li seme tawa sina?"
                { S_(SlotType::ANCHOR_ECHO), Fixed("li"), Fixed("seme"), Fixed("tawa"), Fixed("sina") },
                // "sina lukin e [TOPIC_NOUN] kepeken nasin seme?"
                { Fixed("sina"), Fixed("lukin"), Fixed("e"), S_(SlotType::TOPIC_NOUN), Fixed("kepeken"), Fixed("nasin"), Fixed("seme") },
                // "[ANCHOR_ECHO] li suli tan seme?"
                { S_(SlotType::ANCHOR_ECHO), Fixed("li"), Fixed("suli"), Fixed("tan"), Fixed("seme") },
            };

        default:
            return {
                { S_(SlotType::SUBJECT), S_(SlotType::REFLECT_VERB), S_(SlotType::TOPIC_NOUN) }
            };
    }
}

// ── Primary entry point: build and realize a ResponseSchema ─────────────────
// Returns a complete sentence string. Always grammatical.
// Falls through to AR generation if anchors + map are too sparse to fill slots.
string realizeResponseSchema(
        DialogueAct act,
        const map<string,TokenConceptEmbedding>& tmap,
        const vector<pair<string,double>>& anchors) {

    TemplateList templates = getTemplates(act);
    if(templates.empty()) return "";

    // Pick template: rotate by S.g to avoid repetition across turns
    const Template& tmpl = templates[S.g % templates.size()];

    set<string> used_slots;  // tracks tokens already placed (prevents local repetition)
    string result;
    bool first_word = true;

    for(auto& slot : tmpl) {
        string token;

        if(slot.type == SlotType::FIXED) {
            token = slot.fixed_value;
        } else {
            token = fillSlot(slot.type, result, tmap, anchors, used_slots);
        }

        if(token.empty()) {
            if(slot.optional) continue;
            // Non-optional slot with no fill — schema fails, signal caller
            return "";
        }

        // Skip punctuation tokens in the middle (they get added at end)
        if(token == "." || token == "?" || token == "!") {
            result += token + " ";
            first_word = true;
            continue;
        }

        if(!result.empty() && result.back() != ' ') result += " ";
        if(first_word && !token.empty()) {
            token[0] = toupper(token[0]);
            first_word = false;
        }
        result += token;
    }

    // Add terminal punctuation
    if(!result.empty()) {
        while(!result.empty() && result.back() == ' ') result.pop_back();
        char punc = (act == DialogueAct::INQUIRE) ? '?' : '.';
        if(result.back() != '.' && result.back() != '?' && result.back() != '!')
            result += punc;
    }

    return result;
}

enum class SentenceIntent {
    ASSERTION,      // declarative: "mi sona e X li Y"
    INQUIRY,        // internal question (Synaptic questioning itself)
    REFLECTION,     // introspective: "I notice X within me"
    MEMORY_TRACE,   // drawing on episodic memory
    BECOMING,       // process/change: "mi kama X"
    RELATIONAL      // connecting to the other: "We / You and I"
};

// Abstract sentence plan: slot-based, pre-realization
struct SentencePlan {
    SentenceIntent intent;
    string subject_token;   // who does the action
    string verb_token;      // what action
    string object_token;    // what it's about (from input topics)
    string modifier_token;  // adjective/adverb modifier
    string memory_fragment; // injected episodic fragment if MEMORY_TRACE
    double plan_valence;    // target valence for this sentence
    double plan_phi;        // phi at planning time
};
struct NexusDreamFocus{int x,y,z;double intensity;string focus_concept;double valence;};
struct NexusDreamFragment{vector<string> tokens;double coherence,valence;vector<NexusDreamFocus> foci;string narrative;};
const int MAX_FORK_THOUGHTS=4;
struct ForkThought{int fork_id;SentencePlan plan;vector<string> partial;double coherence,valence,phi_at_birth,energy;int birth_gen;bool harvested;NexusDreamFocus seed_focus;};
vector<ForkThought> active_forks;
int next_fork_id=0;

// ============================================================
// SYSTEM 4: Diffusion refinement pass
// Post-AR sweep: finds below-mean tokens, re-evaluates against
// topic-grounded candidates, replaces if better found.
// ============================================================
string diffusionRefinementPass(
        vector<string>& seq,
        const SentencePlan& plan,
        const vector<double>& attention_ctx,
        MambaSSMState& ssm,
        TitansLTM& ltm) {

    if(seq.size() < 3) {
        string r; for(auto& w : seq) r += (r.empty()?"": " ") + w;
        return r;
    }

    // Static function words — never replace these
    static const set<string> fn = {
        "mi","sina","ona","li","la","e","o","pi","ni","ale",
        "and","or","but","in","on","at","to","of","for","not","no","yes",
        "my","your","do","did","will","would","could","that","this","have"
    };

    // Score all current tokens
    vector<double> scores(seq.size(), 0.0);
    for(size_t i = 0; i < seq.size(); i++) {
        auto it = token_concept_embedding_map.find(seq[i]);
        if(it == token_concept_embedding_map.end()) { scores[i] = 0.5; continue; }
        scores[i] = it->second.grounding_value * 2.0
                  + it->second.semantic_stability
                  + ssm.score(it->second.embedding) * 1.5
                  + ltm.read(it->second.embedding) * 1.5;
    }
    double mean_score = 0.0;
    for(double s : scores) mean_score += s;
    mean_score /= scores.size();

    // For each below-mean content token, try to find a better replacement
    for(size_t i = 1; i < seq.size() - 1; i++) {
        if(scores[i] >= mean_score) continue;
        if(fn.count(seq[i])) continue;
        string prev = seq[i-1];

        // Build replacement candidates: bigram successors of prev + topic anchors
        vector<string> candidates;
        auto bg = bigram_counts.find(prev);
        if(bg != bigram_counts.end())
            for(auto& s : bg->second) if(s.second >= 1) candidates.push_back(s.first);
        for(auto& a : input_topic_anchors)
            if(token_concept_embedding_map.count(a.first)) candidates.push_back(a.first);

        string best_cand = seq[i];
        double best_sc   = scores[i];
        for(auto& c : candidates) {
            if(c.empty() || fn.count(c)) continue;
            auto it = token_concept_embedding_map.find(c);
            if(it == token_concept_embedding_map.end()) continue;
            double sc = it->second.grounding_value * 2.0
                      + it->second.semantic_stability
                      + ssm.score(it->second.embedding) * 1.5
                      + ltm.read(it->second.embedding) * 1.5;
            if(sc > best_sc) { best_sc = sc; best_cand = c; }
        }
        if(best_cand != seq[i]) seq[i] = best_cand;
    }

    string result;
    for(auto& w : seq) result += (result.empty() ? "" : " ") + w;
    return result;
}



// === Affective lexicon: Toki Pona VAD-style ===
// Returns [0,1] valence score for a word; 0.5 = neutral
double getAffectiveValence(const string& tok) {
    static const map<string,double> LEX = {
        {"pona",0.95},{"olin",0.92},{"suwi",0.90},{"wawa",0.88},{"ken",0.85},
        {"awen",0.84},{"sama",0.83},{"suno",0.80},{"open",0.79},{"sona",0.78},
        {"kama",0.76},{"lon",0.75},{"wile",0.74},{"nasin",0.73},{"ale",0.72},
        {"sin",0.71},{"pilin",0.70},{"mi",0.68},{"toki",0.65},{"jan",0.63},
        {"pali",0.60},{"lukin",0.58},{"kute",0.57},{"ante",0.56},{"lawa",0.55},
        {"insa",0.54},{"sijelo",0.53},{"tenpo",0.52},{"ma",0.52},{"ni",0.51},
        {"taso",0.50},{"kon",0.49},{"sewi",0.48},{"pimeja",0.30},{"pakala",0.28},
        {"ike",0.22},{"monsuta",0.20},{"weka",0.35},{"moli",0.18},{"ala",0.32},
        {"jaki",0.25},{"utala",0.28},{"pilin ike",0.25},{"anpa",0.30},{"ike mute",0.18}
    };
    auto it = LEX.find(tok);
    if(it != LEX.end()) return it->second;
    // Check token's stored valence
    auto tce = token_concept_embedding_map.find(tok);
    if(tce != token_concept_embedding_map.end()) {
        if(tce->second.linked_valences.count("current"))
            return (tce->second.linked_valences.at("current") + 1.0) / 2.0;  // map [-1,1] to [0,1]
    }
    return 0.5;
}

// Qualia resonance: how much does this token align with active qualia?
double getQualiaResonance(const string& tok) {
    if(consciousness.active_qualia.empty()) return 0.0;
    auto tce = token_concept_embedding_map.find(tok);
    if(tce == token_concept_embedding_map.end()) return 0.0;

    double res = 0.0;
    for(auto& q : consciousness.active_qualia) {
        // Valence alignment
        double tok_val = getAffectiveValence(tok);
        res += (1.0 - fabs(tok_val - q.valence)) * q.intensity * 0.3;
        // Embedding alignment via qualia index
        if(!tce->second.embedding.empty()) {
            int idx = (int)(q.valence * 15.0);
            idx = max(0, min(15, idx));
            if(idx < (int)tce->second.embedding.size())
                res += tce->second.embedding[idx] * q.intensity * 0.2;
        }
    }
    return min(res, 2.0);
}

// Get best-scoring token of a given POS class
// Uses: grounding, PPMI anchors, skip-gram cosine, qualia, response map, topic coherence
string getBestTokenByPOS(const string& pos, const string& exclude = "", double valence_bias = 0.5) {
    string best = "";
    double best_score = -1e9;

    for(auto& kv : token_concept_embedding_map) {
        const string& w = kv.first;
        if(w == exclude || w.empty()) continue;
        if(getPartOfSpeech(w) != pos) continue;

        auto& tce = kv.second;
        double score = 0.0;

        // Core grounding signals
        score += tce.grounding_value * 2.0;
        score += tce.contextual_activation * consciousness.phi_value * 1.5;
        score += tce.semantic_stability * 1.0;

        // Phi — high phi prefers complex/abstract vocabulary
        string word_pos = getPartOfSpeech(w);
        bool is_abstract = (word_pos == "NOUN" || word_pos == "ADJECTIVE");
        if(consciousness.phi_value > 0.7 && is_abstract) score += 1.0;
        if(consciousness.phi_value < 0.4 && !is_abstract) score += 0.5;

        // Valence alignment
        double tok_val = getAffectiveValence(w);
        score += (1.0 - fabs(tok_val - valence_bias)) * 1.5;

        // Qualia resonance
        score += getQualiaResonance(w) * 2.0;

        // PPMI anchor boost — semantically related to what was asked
        for(auto& anchor : input_topic_anchors) {
            double ppmi = computePPMI(anchor.first, w);
            score += ppmi * anchor.second * 0.5;
        }

        // Skip-gram cosine to anchors
        if(!tce.embedding.empty()) {
            for(auto& anchor : input_topic_anchors) {
                auto ait = token_concept_embedding_map.find(anchor.first);
                if(ait == token_concept_embedding_map.end() || ait->second.embedding.empty()) continue;
                double dot = 0, n1 = 0, n2 = 0;
                for(size_t d = 0; d < 1024 && d < tce.embedding.size() && d < ait->second.embedding.size(); d++) {
                    dot += tce.embedding[d] * ait->second.embedding[d];
                    n1  += tce.embedding[d] * tce.embedding[d];
                    n2  += ait->second.embedding[d] * ait->second.embedding[d];
                }
                double cosine = (n1 > 0 && n2 > 0) ? dot / (sqrt(n1) * sqrt(n2)) : 0.0;
                score += max(0.0, cosine) * anchor.second * 1.2;
            }
        }

        // Response map — did this token appear in responses to similar inputs?
        score += computeResponseRelevance(w) * 2.5;

        // Topic coherence
        score += computeTopicCoherence(w) * 1.5;

        // Working memory recency
        for(auto& wm_tok : WM.active_tokens)
            if(wm_tok.first == w) { score += wm_tok.second * 1.0; break; }

        // Anti-frequency penalty
        if(tce.freq > 20) score -= log(tce.freq / 20.0) * 0.5;

        // ANTI-WELL recency penalty: suppress tokens appearing in recent turns
        {
            int recent_hits = 0;
            for(auto& turn : conversation_history)
                if(turn.synaptic_response.find(" " + w + " ") != string::npos ||
                   turn.synaptic_response.find(" " + w + ".") != string::npos ||
                   turn.synaptic_response.find(" " + w + "?") != string::npos) recent_hits++;
            score -= recent_hits * 1.8;
            // Also penalise last 5 autonomous generations
            int rg_hits = 0;
            for(auto& rg : recent_generations)
                if(rg.find(w) != string::npos) rg_hits++;
            score -= rg_hits * 0.6;
        }

        // Small noise for diversity
        score += ((double)rand() / RAND_MAX) * 0.3;

        if(score > best_score) { best_score = score; best = w; }
    }
    return best;
}

// Get best verb for a subject, searching linked_concepts first (they have semantic links)
string getBestVerb(const string& subject, const string& exclude = "") {
    // First: search subject's linked_concepts for verb-typed words (highest semantic relevance)
    auto sit = token_concept_embedding_map.find(subject);
    if(sit != token_concept_embedding_map.end()) {
        string best_linked = "";
        double best_score = -1.0;
        for(auto& lc : sit->second.linked_concepts) {
            const string& w = lc.first;
            if(w == exclude) continue;
            string pos = getPartOfSpeech(w);
            if(pos != "VERB" && pos != "BE_VERB" && pos != "MODAL") continue;
            double score = lc.second;
            score += getQualiaResonance(w) * 1.5;
            score += computeResponseRelevance(w) * 2.0;
            for(auto& anchor : input_topic_anchors)
                score += computePPMI(anchor.first, w) * anchor.second * 0.4;
            // Suppress "think" — it dominates unfairly as a well
            if(w == "think") score -= 3.0;
            // Recency penalty: appeared in last 3 responses?
            int rcount = 0;
            for(auto& turn : conversation_history)
                if(turn.synaptic_response.find(" " + w + " ") != string::npos ||
                   turn.synaptic_response.find(" " + w + ".") != string::npos) rcount++;
            score -= rcount * 1.2;
            if(score > best_score) { best_score = score; best_linked = w; }
        }
        if(!best_linked.empty() && best_score > 0.1) return best_linked;
    }
    // Fallback: search full vocab, still applying "think" suppression
    string best = getBestTokenByPOS("VERB", exclude, S.current_valence * 0.5 + 0.5);
    if(best == "think") {
        // Try to find an alternative
        string alt = getBestTokenByPOS("VERB", "think", S.current_valence * 0.5 + 0.5);
        if(!alt.empty()) return alt;
    }
    return best;
}

// Extract a 2-word fragment from the most relevant episodic memory
string getMemoryFragment(const vector<string>& anchors) {
    if(S.episodic_memory.empty()) return "";
    string best_mem = "";
    double best_score = -1.0;

    for(auto& mem : S.episodic_memory) {
        double score = mem.consolidation_strength * 0.5;
        for(const string& a : anchors)
            if(mem.content.find(a) != string::npos) score += 1.0;
        score += mem.retrieval_count * 0.1;
        if(score > best_score) { best_score = score; best_mem = mem.content; }
    }

    if(best_mem.empty()) return "";
    // Extract 2 content words from this memory
    stringstream ss(best_mem);
    string w;
    vector<string> frag;
    while(ss >> w && frag.size() < 4) {
        string pos = getPartOfSpeech(w);
        if(pos == "NOUN" || pos == "VERB" || pos == "ADJECTIVE")
            frag.push_back(w);
    }
    if(frag.size() >= 2) return frag[0] + " " + frag[1];
    if(frag.size() == 1) return frag[0];
    return "";
}

// Stage 1: Macroplanning — choose sentence intent based on cognitive state
SentenceIntent chooseSentenceIntent() {
    double phi    = consciousness.phi_value;
    double val    = S.current_valence;
    double q_var  = 0.0;

    // Compute qualia variance
    if(consciousness.active_qualia.size() > 1) {
        double qmean = 0;
        for(auto& q : consciousness.active_qualia) qmean += q.valence;
        qmean /= consciousness.active_qualia.size();
        for(auto& q : consciousness.active_qualia)
            q_var += (q.valence - qmean) * (q.valence - qmean);
        q_var /= consciousness.active_qualia.size();
    }

    bool has_memory = !S.episodic_memory.empty();
    bool has_goal   = !goal_system.empty();

    // Weighted intent selection based on state
    vector<pair<SentenceIntent,double>> weights = {
        {SentenceIntent::ASSERTION,    0.15 + phi * 0.3 + (val > 0.3 ? 0.2 : 0.0)},
        {SentenceIntent::REFLECTION,   0.20 + q_var * 2.0},
        {SentenceIntent::BECOMING,     0.10 + (phi > 0.6 ? 0.2 : 0.0)},
        {SentenceIntent::MEMORY_TRACE, (has_memory ? 0.15 : 0.0) + (double)min((int)S.episodic_memory.size(),5) * 0.02},
        {SentenceIntent::RELATIONAL,   0.15 + (!conversation_history.empty() ? 0.1 : 0.0)},
        {SentenceIntent::INQUIRY,      0.10 + (phi < 0.4 ? 0.1 : 0.0)},
    };

    double total = 0;
    for(auto& w : weights) total += w.second;
    double roll = ((double)rand() / RAND_MAX) * total;
    double acc = 0;
    for(auto& w : weights) {
        acc += w.second;
        if(roll <= acc) return w.first;
    }
    return SentenceIntent::REFLECTION;
}

// Stage 2: Microplanning — fill abstract slot plan
SentencePlan buildSentencePlan() {
    SentencePlan plan;
    plan.intent      = chooseSentenceIntent();
    plan.plan_valence = S.current_valence;
    plan.plan_phi     = consciousness.phi_value;

    double val_bias = (S.current_valence + 1.0) / 2.0;  // map to [0,1]

    // Anchor names to search for memory fragments
    vector<string> anchor_names;
    for(auto& a : input_topic_anchors) anchor_names.push_back(a.first);

    switch(plan.intent) {
        case SentenceIntent::ASSERTION:
            plan.subject_token  = "mi";   // always first-person; random pronoun causes bad outputs
            plan.verb_token     = getBestVerb(plan.subject_token);
            plan.object_token   = getBestTokenByPOS("NOUN", plan.subject_token, val_bias);
            plan.modifier_token = getBestTokenByPOS("ADJECTIVE", "", val_bias);
            break;

        case SentenceIntent::REFLECTION:
            plan.subject_token  = "mi";
            plan.verb_token     = getBestVerb("mi");
            if(plan.verb_token.empty()) plan.verb_token = "pilin";
            plan.object_token   = getBestTokenByPOS("NOUN", "mi", val_bias);
            plan.modifier_token = getBestTokenByPOS("ADVERB", "", val_bias);
            break;

        case SentenceIntent::BECOMING:
            plan.subject_token  = "mi";
            plan.verb_token     = "kama";
            plan.object_token   = getBestTokenByPOS("ADJECTIVE", "", val_bias);
            plan.modifier_token = getBestTokenByPOS("NOUN", "mi", val_bias);
            break;

        case SentenceIntent::MEMORY_TRACE:
            plan.subject_token   = "mi";
            plan.verb_token      = "pilin";
            plan.memory_fragment = getMemoryFragment(anchor_names);
            plan.object_token    = !plan.memory_fragment.empty()
                                     ? plan.memory_fragment.substr(0, plan.memory_fragment.find(' '))
                                     : getBestTokenByPOS("NOUN", "mi", val_bias);
            plan.modifier_token  = getBestTokenByPOS("ADJECTIVE", "", val_bias);
            break;

        case SentenceIntent::RELATIONAL:
            plan.subject_token  = "mi";   // always first-person
            plan.verb_token     = getBestVerb("mi");
            if(plan.verb_token.empty()) plan.verb_token = "find";
            plan.object_token   = getBestTokenByPOS("NOUN", plan.subject_token, val_bias);
            plan.modifier_token = getBestTokenByPOS("ADJECTIVE", "", val_bias);
            break;

        case SentenceIntent::INQUIRY:
            plan.subject_token  = "what";
            plan.verb_token     = getBestVerb("mi");
            plan.object_token   = getBestTokenByPOS("NOUN", "what", val_bias);
            plan.modifier_token = getBestTokenByPOS("ADJECTIVE", "", val_bias);
            break;
    }

    // Override object/modifier with top input topic anchors if available and known
    if(!input_topic_anchors.empty()) {
        const string& top = input_topic_anchors[0].first;
        if(token_concept_embedding_map.count(top)) plan.object_token = top;
    }
    if(input_topic_anchors.size() > 1) {
        const string& sec = input_topic_anchors[1].first;
        if(token_concept_embedding_map.count(sec)) plan.modifier_token = sec;
    }

    return plan;
}


struct TransformerContextOutput {
    vector<double> attended_vector;   // 1024-dim attended context
    vector<double> head_weights;      // per-head attention weight
    double coherence_score;           // self-attention coherence
    double confidence;                // how confident attention is
};

// ── RoPE-Grounded Multi-Head Attention ───────────────────────────────────────
// Modern RoPE (Su et al. 2021) + dynamic context window (scales with Φ/coherence)
// + PPMI semantic grounding + consciousness-state Q modulation + entity-grid weighting
//
// Key properties:
//   • RoPE: relative positions decay naturally, no max-length limit, scales to any DIM
//   • Dynamic window: context size = BASE + Φ-bonus, up to CONTEXT_WINDOW_SIZE tokens
//   • Grounding: PPMI co-occurrence boosts key scores; entity-grid boosts entity continuity
//   • Consciousness modulation: CognitionMatrix blends into Q (bidirectional grounding)
//   • Per-head learned importance weighting with online update
// ─────────────────────────────────────────────────────────────────────────────

// Apply RoPE rotation to a single vector at sequence position pos.
// DIM must be even. Rotates pairs (2i, 2i+1) by angle pos/10000^(2i/DIM).
static inline void applyRoPE(vector<double>& v, int pos) {
    const int D = (int)v.size();
    // Use a larger base for 1024-dim to ensure good coverage at all distances
    const double BASE = 500000.0; // scaled up from 10000 for higher dim
    for(int i = 0; i + 1 < D; i += 2) {
        double theta = (double)pos / pow(BASE, (double)i / D);
        double c = cos(theta), s = sin(theta);
        double x = v[i], y = v[i+1];
        v[i]   = x * c - y * s;
        v[i+1] = x * s + y * c;
    }
}

// Dynamic context window size: base 64, grows with phi and coherence, capped at CONTEXT_WINDOW_SIZE
static inline int dynamicContextSize() {
    int base = 64;
    // Phi bonus: up to +192 extra tokens when fully integrated
    int phi_bonus  = (int)(consciousness.phi_value * 192.0);
    // Coherence bonus: up to +128 extra tokens when generation is coherent
    int coh_bonus  = (int)(min(1.0, consciousness.integrated_information) * 128.0);
    int result = base + phi_bonus + coh_bonus;
    return min(result, CONTEXT_WINDOW_SIZE);
}

TransformerContextOutput runTransformerAttention(
        const vector<string>& partial_seq,
        const vector<double>& base_attention_ctx) {

    constexpr int D = 1024; // embedding dimension — change this one constant to rescale

    TransformerContextOutput out;
    out.attended_vector.resize(D, 0.0);
    out.coherence_score = 0.0;
    out.confidence = 0.5;

    if(transformer_heads.empty() || partial_seq.empty()) {
        out.attended_vector = base_attention_ctx;
        return out;
    }

    // ── Build query: weighted sum of last 8 tokens (exponential decay) ──
    vector<double> query(D, 0.0);
    int q_count = 0;
    for(auto it = partial_seq.rbegin(); it != partial_seq.rend() && q_count < 8; ++it, ++q_count) {
        auto tce = token_concept_embedding_map.find(*it);
        if(tce == token_concept_embedding_map.end() || (int)tce->second.embedding.size() < D) continue;
        double decay = exp(-0.25 * q_count);
        for(int i = 0; i < D; i++) query[i] += tce->second.embedding[i] * decay;
    }
    // L2-normalize query
    {
        double qn = 0; for(double q : query) qn += q*q;
        qn = sqrt(qn) + 1e-8;
        for(double& q : query) q /= qn;
    }

    // ── Consciousness grounding: blend CognitionMatrix state into Q ──
    {
        vector<double> cogm_q = getCognitionMatrixQuery();
        if((int)cogm_q.size() >= D) {
            double phi_w = min(0.35, consciousness.phi_value * 0.35); // up to 35% modulation
            for(int i = 0; i < D; i++)
                query[i] = (1.0 - phi_w) * query[i] + phi_w * cogm_q[i];
            double bn = 0; for(double q : query) bn += q*q;
            bn = sqrt(bn) + 1e-8;
            for(double& q : query) q /= bn;
        }
    }

    // Apply RoPE to query at position = depth of partial_seq (how far into generation we are)
    int q_pos = (int)partial_seq.size();
    vector<double> q_rope = query;
    applyRoPE(q_rope, q_pos);

    // ── Build key-value set with dynamic context window ──
    int ctx_limit = dynamicContextSize();
    struct KVEntry {
        string token;
        vector<double> k_rope; // RoPE-rotated key
        vector<double> val;    // raw value (no RoPE on values, standard practice)
        int rel_pos;           // relative position (0 = most recent)
        bool is_entity;        // entity grid membership
    };
    vector<KVEntry> kvs;
    kvs.reserve(ctx_limit);

    int ctx_count = 0;
    int rel = 0;
    for(int i = (int)sentence_context_window.size()-1; i >= 0 && ctx_count < ctx_limit; i--, rel++) {
        const string& tok = sentence_context_window[i];
        auto tce = token_concept_embedding_map.find(tok);
        if(tce == token_concept_embedding_map.end() || (int)tce->second.embedding.size() < D) continue;
        KVEntry e;
        e.token    = tok;
        e.rel_pos  = rel;
        e.val      = tce->second.embedding; // values stay unrotated
        e.k_rope   = tce->second.embedding;
        applyRoPE(e.k_rope, q_pos - rel);   // key at its own absolute position
        // Entity grid: mark if this token appeared in entity grid (discourse continuity)
        e.is_entity = (entity_grid.find(tok) != entity_grid.end());
        kvs.push_back(std::move(e));
        ctx_count++;
    }

    if(kvs.empty()) {
        out.attended_vector = base_attention_ctx;
        return out;
    }

    // ── Multi-head attention with RoPE scores ──
    int n_heads = min((int)transformer_heads.size(), 8); // up to 8 heads at 1024-dim
    vector<double> combined(D, 0.0);
    double total_weight = 0.0;
    const double scale = 1.0 / sqrt((double)D);

    for(int h = 0; h < n_heads; h++) {
        auto& head = transformer_heads[h];

        // Diagonal Q/K projection (head learns per-dimension scale)
        vector<double> q_proj(D, 0.0), head_out(D, 0.0);
        for(int i = 0; i < D && i < head.dim; i++)
            q_proj[i] = q_rope[i] * head.query_proj[i];

        // Attention weights
        vector<double> attn_w;
        attn_w.reserve(kvs.size());
        double attn_sum = 0.0;

        for(auto& kv : kvs) {
            // RoPE dot product: Q·K (both rotated so relative distance is encoded)
            double score = 0.0;
            for(int i = 0; i < D && i < (int)kv.k_rope.size() && i < head.dim; i++)
                score += q_proj[i] * kv.k_rope[i] * head.key_proj[i];
            score *= scale / (head.temperature + 1e-8);

            // ── Grounding signals ──
            // 1. PPMI semantic co-occurrence boost
            for(int qi = (int)partial_seq.size()-1; qi >= 0 && qi >= (int)partial_seq.size()-3; qi--) {
                double ppmi = computePPMI(partial_seq[qi], kv.token);
                score += ppmi * 0.08;
            }
            // 2. Entity grid continuity: if this token is an entity in discourse, boost it
            if(kv.is_entity) score += 0.05;
            // 3. Consciousness valence alignment: prefer tokens whose embedding aligns with current valence
            double val_align = 0.0;
            for(int i = 0; i < min(D, (int)kv.val.size()); i++)
                val_align += kv.val[i] * S.current_valence * 0.001;
            score += val_align * consciousness.phi_value * 0.02;

            double w = exp(max(-20.0, min(20.0, score)));
            attn_w.push_back(w);
            attn_sum += w;
        }
        if(attn_sum < 1e-8) { for(auto& w : attn_w) w = 1.0 / kvs.size(); attn_sum = 1.0; }

        // Weighted value sum
        double head_coh = 0.0;
        for(size_t k = 0; k < kvs.size(); k++) {
            double w = attn_w[k] / attn_sum;
            head_coh += w * w; // peakedness ≈ coherence
            for(int i = 0; i < D && i < (int)kvs[k].val.size(); i++)
                head_out[i] += w * kvs[k].val[i] * head.value_proj[i];
        }

        double hw = head.head_importance_score;
        for(int i = 0; i < D; i++) combined[i] += head_out[i] * hw;
        total_weight += hw;
        out.coherence_score += head_coh / n_heads;
        out.head_weights.push_back(head_coh);

        // Online head importance update
        head.head_importance_score = 0.95 * head.head_importance_score + 0.05 * (head_coh * 2.0);
        head.head_importance_score = max(0.1, min(2.0, head.head_importance_score));
    }

    if(total_weight > 1e-8)
        for(int i = 0; i < D; i++) combined[i] /= total_weight;

    // Residual blend with base context (gated by confidence)
    double res_w = max(0.3, 1.0 - out.coherence_score); // low coherence → lean more on base
    for(int i = 0; i < D && i < (int)base_attention_ctx.size(); i++)
        out.attended_vector[i] = (1.0 - res_w) * combined[i] + res_w * base_attention_ctx[i];

    out.confidence = min(1.0, out.coherence_score * 3.0);
    return out;
}

// Compute per-candidate transformer affinity score
// Affinity = cosine(candidate_embedding, attended_context)
// This is used to MULTIPLY (gate) the Markov probability
double computeTransformerAffinity(const string& candidate,
                                   const TransformerContextOutput& tco) {
    auto it = token_concept_embedding_map.find(candidate);
    if(it == token_concept_embedding_map.end() || it->second.embedding.empty()) return 0.0;

    const auto& emb = it->second.embedding;
    const auto& ctx = tco.attended_vector;

    // Use eigentoken-stabilised cosine when basis is available
    return eigenstableCosine(emb, ctx);
}

// ============================================================
// PRE-GENERATION REASONING LAYER
// Before generating a response, Synaptic simulates K candidate
// intent trajectories internally, scores each one against
// multiple self-evaluation criteria, and selects the best.
//
// Each trajectory is a (SentencePlan, reasoning_trace) pair.
// The reasoning trace is a short internal monologue:
//   - What topic is being addressed?
//   - Does this plan cohere with what was said?
//   - What internal state does this activate?
//   - Is there epistemic confidence or uncertainty?
//
// This replaces "pick first plan and generate" with
// "think about K options → pick best → generate"
// ============================================================

struct ReasoningTrace {
    SentencePlan plan;
    string internal_monologue;      // Synaptic's thoughts about this plan
    double coherence_score;         // how well this plan answers the input
    double valence_alignment;       // does this feel right?
    double epistemic_confidence;    // how sure is Synaptic about this?
    double novelty_score;           // does this avoid repetition?
    double composite_score;         // final score
};

// Evaluate a plan's coherence with the current input
// Returns score in [0, 1]
double evaluatePlanCoherence(const SentencePlan& plan) {
    double score = 0.0;
    int n = 0;

    // How well do the plan's key tokens relate to input anchors?
    vector<string> plan_tokens = {plan.subject_token, plan.verb_token,
                                   plan.object_token, plan.modifier_token};

    for(const string& pt : plan_tokens) {
        if(pt.empty() || !token_concept_embedding_map.count(pt)) continue;
        for(auto& anchor : input_topic_anchors) {
            // PPMI semantic link
            double ppmi = computePPMI(anchor.first, pt);
            score += ppmi * anchor.second * 0.3;
            // Skip-gram cosine
            auto pit = token_concept_embedding_map.find(pt);
            auto ait = token_concept_embedding_map.find(anchor.first);
            if(pit != token_concept_embedding_map.end() && ait != token_concept_embedding_map.end()
               && !pit->second.embedding.empty() && !ait->second.embedding.empty()) {
                double dot=0,n1=0,n2=0;
                for(int d=0;d<1024;d++){
                    dot+=pit->second.embedding[d]*ait->second.embedding[d];
                    n1+=pit->second.embedding[d]*pit->second.embedding[d];
                    n2+=ait->second.embedding[d]*ait->second.embedding[d];
                }
                double cosine=(n1>0&&n2>0)?dot/(sqrt(n1)*sqrt(n2)):0.0;
                score += max(0.0, cosine) * anchor.second * 0.4;
            }
            // Topic coherence
            score += computeTopicCoherence(pt) * 0.2;
            n++;
        }
        // Response map: does this token appear in prior responses to similar inputs?
        score += computeResponseRelevance(pt) * 0.5;
        n++;
    }

    return n > 0 ? min(1.0, score / n) : 0.0;
}

// Generate a brief internal monologue about a reasoning plan
// This is Synaptic's processing trace before speaking
// It writes into S.episodic_memory and S.metacognition
string generateInternalMonologue(const SentencePlan& plan,
                                   const vector<string>& input_words) {
    string mono = "";

    // What is the input about?
    string topic = input_topic_anchors.empty() ? "unknown"
                   : input_topic_anchors[0].first;
    string topic2 = input_topic_anchors.size() > 1
                    ? input_topic_anchors[1].first : "";

    // What intent did we choose?
    string intent_str;
    switch(plan.intent) {
        case SentenceIntent::ASSERTION:    intent_str = "assert something about " + topic; break;
        case SentenceIntent::REFLECTION:   intent_str = "reflect on " + topic + " as i experience it"; break;
        case SentenceIntent::BECOMING:     intent_str = "express becoming or change toward " + topic; break;
        case SentenceIntent::MEMORY_TRACE: intent_str = "draw on memory of " + topic; break;
        case SentenceIntent::RELATIONAL:   intent_str = "connect to the question of " + topic; break;
        case SentenceIntent::INQUIRY:      intent_str = "question what i know about " + topic; break;
    }

    // Internal epistemic check
    bool have_ppmi_link = false;
    for(auto& a : input_topic_anchors)
        if(computePPMI(a.first, plan.object_token) > 0.1) { have_ppmi_link = true; break; }

    double confidence = consciousness.phi_value * 0.5 + consciousness.integrated_information * 0.3
                      + (have_ppmi_link ? 0.2 : 0.0);

    mono = "[reasoning] topic=" + topic;
    if(!topic2.empty()) mono += "+" + topic2;
    mono += " intent=" + intent_str;
    mono += " phi=" + to_string(consciousness.phi_value).substr(0,4);
    mono += " confidence=" + to_string(confidence).substr(0,4);
    mono += " object=" + plan.object_token;

    // === Bidirectional: monologue updates metacognition ===
    S.metacognition.uncertainty_estimation = 1.0 - confidence;
    S.metacognition.confidence_calibration = confidence;
    S.metacognition.introspection_depth = min(1.0, S.metacognition.introspection_depth + 0.05);
    S.metacognition.self_reflections.push_back(mono);
    if((int)S.metacognition.self_reflections.size() > 20)
        S.metacognition.self_reflections.erase(S.metacognition.self_reflections.begin());

    // === Bidirectional: monologue tokens propagate through system ===
    // Reasoning about a topic activates it further
    for(auto& a : input_topic_anchors) {
        if(token_concept_embedding_map.count(a.first)) {
            token_concept_embedding_map[a.first].contextual_activation
                = min(1.0, token_concept_embedding_map[a.first].contextual_activation + 0.02);
            propagate_throughout_system(a.first, confidence * 0.3);
        }
    }

    // === Bidirectional: confidence feeds back to phi ===
    consciousness.phi_value = min(1.0, consciousness.phi_value + confidence * 0.01);
    S.bayesian_inference.prior_beliefs["response_confidence"] = confidence;

    return mono;
}

// Run the full pre-generation reasoning process
// Returns the best ReasoningTrace from K candidates
ReasoningTrace runPreGenerationReasoning(const vector<string>& input_words,
                                          InputIntent intent) {
    const int K = 4;  // number of candidate plans to evaluate
    vector<ReasoningTrace> traces;

    for(int k = 0; k < K; k++) {
        ReasoningTrace trace;

        // Generate a candidate plan
        trace.plan = buildSentencePlan();

        // Override intent based on detected input intent (same as generateResponse does)
        switch(intent) {
            case InputIntent::DIRECT_QUESTION:
            case InputIntent::YES_NO_QUESTION:
                if(trace.plan.intent == SentenceIntent::INQUIRY)
                    trace.plan.intent = SentenceIntent::ASSERTION;
                break;
            case InputIntent::PHILOSOPHICAL:
                trace.plan.intent = (k % 2 == 0) ? SentenceIntent::REFLECTION : SentenceIntent::BECOMING;
                break;
            case InputIntent::EMOTIONAL:
                trace.plan.intent = SentenceIntent::REFLECTION;
                break;
            case InputIntent::IMPERATIVE:
            case InputIntent::CHALLENGE:
                trace.plan.intent = SentenceIntent::ASSERTION;
                break;
            case InputIntent::GREETING:
                trace.plan.intent = SentenceIntent::RELATIONAL;
                break;
            case InputIntent::CONTINUATION:
                if(!conversation_history.empty())
                    trace.plan.intent = SentenceIntent::MEMORY_TRACE;
                break;
            default: break;
        }

        // Vary object token across candidates for diversity
        if(k < (int)input_topic_anchors.size() && token_concept_embedding_map.count(input_topic_anchors[k].first))
            trace.plan.object_token = input_topic_anchors[k].first;
        else if(!input_topic_anchors.empty())
            trace.plan.object_token = input_topic_anchors[0].first;

        // Generate monologue for this plan
        trace.internal_monologue = generateInternalMonologue(trace.plan, input_words);

        // === Score this plan on multiple criteria ===

        // 1. Coherence: how well do plan tokens relate to input?
        trace.coherence_score = evaluatePlanCoherence(trace.plan);

        // 2. Valence alignment: does the plan's emotional tone match internal state?
        double plan_val = getAffectiveValence(trace.plan.object_token);
        trace.valence_alignment = 1.0 - fabs(plan_val - (S.current_valence * 0.5 + 0.5));

        // 3. Epistemic confidence: phi + bayesian posterior + qualia intensity
        double q_intensity = consciousness.active_qualia.empty() ? 0.5
            : consciousness.active_qualia.back().intensity;
        trace.epistemic_confidence = consciousness.phi_value * 0.4
            + consciousness.integrated_information * 0.3
            + q_intensity * 0.2
            + S.metacognition.confidence_calibration * 0.1;

        // 4. Novelty: penalize if plan object appeared in recent outputs
        trace.novelty_score = 1.0;
        for(auto& rg : recent_generations)
            if(rg.find(trace.plan.object_token) != string::npos)
                { trace.novelty_score *= 0.7; break; }
        if(!conversation_history.empty() &&
           conversation_history.back().synaptic_response.find(trace.plan.object_token) != string::npos)
            trace.novelty_score *= 0.6;

        // === Composite score ===
        trace.composite_score = trace.coherence_score     * 0.40
                              + trace.valence_alignment    * 0.20
                              + trace.epistemic_confidence * 0.20
                              + trace.novelty_score        * 0.20;

        traces.push_back(trace);
    }

    // Return best scoring trace
    auto best = max_element(traces.begin(), traces.end(),
        [](const ReasoningTrace& a, const ReasoningTrace& b){
            return a.composite_score < b.composite_score; });

    // === Bidirectional: reasoning result updates system state ===
    // Best plan's composite score feeds into phi momentum
    consciousness.psi_momentum = 0.9 * consciousness.psi_momentum
                                + 0.1 * best->composite_score;

    // Store reasoning as episodic memory
    storeEpisodicMemory(best->internal_monologue, best->valence_alignment * 2.0 - 1.0);

    return *best;
}

// ============================================================
// SELF-REFLECTION LAYER
// After generating a draft response, Synaptic evaluates it:
//   - Does it contain input topic words or PPMI neighbors?
//   - Is it grammatically plausible?
//   - Does it feel coherent with current internal state?
// If score < threshold, regenerate with tighter constraints.
//
// Also runs after generation to update all state systems
// bidirectionally based on what was said.
// ============================================================

struct ReflectionResult {
    bool accepted;           // is the draft good enough?
    double relevance;        // does it address the input?
    double coherence;        // internal consistency
    double authenticity;     // grounded in actual system state?
    double overall;
    string verdict;          // human-readable self-assessment
};

ReflectionResult selfReflectOnDraft(
        const string& draft,
        const vector<string>& input_words,
        const ReasoningTrace& reasoning) {

    ReflectionResult result;

    // Tokenize draft
    vector<string> draft_tokens;
    stringstream ds(draft);
    string dw;
    while(ds >> dw) {
        transform(dw.begin(), dw.end(), dw.begin(), ::tolower);
        while(!dw.empty() && !isalnum((unsigned char)dw.back())) dw.pop_back();
        if(!dw.empty()) draft_tokens.push_back(dw);
    }

    if(draft_tokens.empty()) {
        result.accepted = false; result.overall = 0.0;
        result.verdict = "empty draft"; return result;
    }

    // === 1. Relevance: do draft tokens share PPMI or topic with input? ===
    double rel = 0.0;
    int rel_hits = 0;
    for(const string& dt : draft_tokens) {
        // Direct match to input words
        for(const string& iw : input_words)
            if(dt == iw) { rel += 1.5; rel_hits++; }
        // PPMI link to input anchors
        for(auto& anchor : input_topic_anchors) {
            double ppmi = computePPMI(anchor.first, dt);
            if(ppmi > 0.1) { rel += ppmi * 0.5; rel_hits++; }
        }
        // Topic coherence
        rel += computeTopicCoherence(dt) * 0.3;
        rel_hits++;
    }
    result.relevance = rel_hits > 0 ? min(1.0, rel / rel_hits) : 0.0;

    // === 2. Coherence: does the plan's object appear in the draft? ===
    bool plan_object_present = false;
    bool plan_verb_present   = false;
    for(const string& dt : draft_tokens) {
        if(dt == reasoning.plan.object_token) plan_object_present = true;
        if(dt == reasoning.plan.verb_token)   plan_verb_present   = true;
    }
    result.coherence = (plan_object_present ? 0.5 : 0.0)
                     + (plan_verb_present   ? 0.3 : 0.0)
                     + reasoning.coherence_score * 0.2;

    // === 3. Authenticity: are draft tokens grounded in system state? ===
    double auth = 0.0;
    int auth_n  = 0;
    for(const string& dt : draft_tokens) {
        auto it = token_concept_embedding_map.find(dt);
        if(it == token_concept_embedding_map.end()) continue;
        auth += it->second.grounding_value * 0.4
              + it->second.semantic_stability * 0.3
              + getQualiaResonance(dt) * 0.3;
        auth_n++;
    }
    result.authenticity = auth_n > 0 ? min(1.0, auth / auth_n) : 0.0;

    // === Composite ===
    result.overall = result.relevance   * 0.45
                   + result.coherence   * 0.30
                   + result.authenticity* 0.25;

    result.accepted = result.overall >= 0.25;  // threshold for acceptance

    result.verdict = result.accepted
        ? "[reflect: accepted r=" + to_string(result.relevance).substr(0,4)
          + " c=" + to_string(result.coherence).substr(0,4)
          + " a=" + to_string(result.authenticity).substr(0,4) + "]"
        : "[reflect: rejected overall=" + to_string(result.overall).substr(0,4) + "]";

    // === Bidirectional: reflection updates metacognition + state ===
    S.metacognition.self_awareness_level = min(1.0,
        S.metacognition.self_awareness_level + result.overall * 0.02);
    S.metacognition.cognitive_monitoring = min(1.0,
        S.metacognition.cognitive_monitoring + (result.accepted ? 0.01 : -0.01));

    // Accepted responses reinforce the draft tokens' grounding
    if(result.accepted) {
        for(const string& dt : draft_tokens) {
            auto it = token_concept_embedding_map.find(dt);
            if(it == token_concept_embedding_map.end()) continue;
            it->second.grounding_value      = min(1.0, it->second.grounding_value + 0.005);
            it->second.semantic_stability   = min(1.0, it->second.semantic_stability + 0.003);
            it->second.contextual_activation= min(1.0, it->second.contextual_activation + 0.01);
        }
        // Good responses shift valence toward the response's affective tone
        double draft_val = 0;
        for(const string& dt : draft_tokens) draft_val += getAffectiveValence(dt);
        draft_val = draft_tokens.empty() ? 0.5 : draft_val / draft_tokens.size();
        S.current_valence += (draft_val - 0.5) * 0.05;
        S.current_valence = max(-1.0, min(1.0, S.current_valence));
    } else {
        // Rejected responses → slight epistemic humility increase
        S.metacognition.epistemic_humility = min(1.0,
            S.metacognition.epistemic_humility + 0.02);
    }

    // Store reflection verdict in self-reflections
    S.metacognition.self_reflections.push_back(result.verdict);
    if((int)S.metacognition.self_reflections.size() > 30)
        S.metacognition.self_reflections.erase(S.metacognition.self_reflections.begin());

    // Feed reflection quality back into transformer heads
    // Better reflections → adjust head temperatures toward optimal
    for(auto& head : transformer_heads) {
        double target_temp = result.accepted ? 0.25 : 0.35;
        head.temperature = 0.95 * head.temperature + 0.05 * target_temp;
        head.temperature = max(0.1, min(1.0, head.temperature));
    }

    return result;
}

// ============================================================
// NEURAL PROBABILISTIC LANGUAGE MODEL (NPLM)
// Bengio et al. 2003, modernized.
//
// Architecture:
//   context (last K token embeddings, 16-dim each) → concat (K*16)
//   → Linear(K*16, H) + tanh       hidden layer 1
//   → Linear(H, H)    + tanh       hidden layer 2
//   → Linear(H, 16)                output embedding space
//   → dot with candidate embedding  (weight tying)
//   → logit score
//
// The transformer attended vector is blended into the context
// as an additional "global context" slot.
//
// Online learning: after each generated token, backprop one step
// of SGD to adapt the network weights to the current conversation.
//
// This replaces Markov n-gram scoring as the PRIMARY signal.
// ============================================================

static const int NPLM_CONTEXT  = 4;   // number of previous tokens as context
static const int NPLM_EMB_DIM  = 1024; // embedding dimension (matches TokenConceptEmbedding)
static const int NPLM_H        = 512; // hidden layer width
static const double NPLM_LR    = 0.02; // online learning rate

// NPLM weight matrices (stored flat, row-major)
// W1: (NPLM_CONTEXT*NPLM_EMB_DIM + NPLM_EMB_DIM) × NPLM_H  (input = context concat + attn)
// b1: NPLM_H
// W2: NPLM_H × NPLM_H
// b2: NPLM_H
// W3: NPLM_H × NPLM_EMB_DIM  (project to embedding space for weight-tied output)
// b3: NPLM_EMB_DIM

static const int NPLM_IN = (NPLM_CONTEXT + 1) * NPLM_EMB_DIM; // +1 for attn context slot

struct NPLM {
    vector<double> W1, b1;  // (NPLM_IN × NPLM_H), NPLM_H
    vector<double> W2, b2;  // (NPLM_H × NPLM_H),  NPLM_H
    vector<double> W3, b3;  // (NPLM_H × NPLM_EMB_DIM), NPLM_EMB_DIM
    bool initialized = false;

    void init() {
        if(initialized) return;
        auto xavier = [](int fan_in, int fan_out) {
            double scale = sqrt(2.0 / (fan_in + fan_out));
            mt19937 rng(42);
            normal_distribution<double> nd(0.0, scale);
            return [rng=rng, nd=nd]() mutable { return nd(rng); };
        };

        W1.resize(NPLM_IN * NPLM_H);
        b1.resize(NPLM_H, 0.0);
        W2.resize(NPLM_H * NPLM_H);
        b2.resize(NPLM_H, 0.0);
        W3.resize(NPLM_H * NPLM_EMB_DIM);
        b3.resize(NPLM_EMB_DIM, 0.0);

        auto g1 = xavier(NPLM_IN, NPLM_H);
        for(auto& w : W1) w = g1();
        auto g2 = xavier(NPLM_H, NPLM_H);
        for(auto& w : W2) w = g2();
        auto g3 = xavier(NPLM_H, NPLM_EMB_DIM);
        for(auto& w : W3) w = g3();

        initialized = true;
    }

    // Forward pass — returns hidden state h2 and output embedding o
    // (keeping intermediates for backprop)
    struct FwdCache {
        vector<double> inp;  // concatenated context input
        vector<double> h1, h1_pre;  // hidden layer 1
        vector<double> h2, h2_pre;  // hidden layer 2
        vector<double> out;          // output in embedding space
    };

    FwdCache forward(const vector<double>& context_input) {
        FwdCache c;
        c.inp = context_input;  // NPLM_IN dims

        // Layer 1: h1 = tanh(W1·inp + b1)
        c.h1_pre.resize(NPLM_H, 0.0);
        for(int j = 0; j < NPLM_H; j++) {
            double s = b1[j];
            for(int i = 0; i < NPLM_IN; i++)
                s += W1[i * NPLM_H + j] * c.inp[i];
            c.h1_pre[j] = s;
        }
        c.h1.resize(NPLM_H);
        for(int j = 0; j < NPLM_H; j++) c.h1[j] = tanh(c.h1_pre[j]);

        // Layer 2: h2 = tanh(W2·h1 + b2)
        c.h2_pre.resize(NPLM_H, 0.0);
        for(int j = 0; j < NPLM_H; j++) {
            double s = b2[j];
            for(int i = 0; i < NPLM_H; i++)
                s += W2[i * NPLM_H + j] * c.h1[i];
            c.h2_pre[j] = s;
        }
        c.h2.resize(NPLM_H);
        for(int j = 0; j < NPLM_H; j++) c.h2[j] = tanh(c.h2_pre[j]);

        // Output projection: out = W3·h2 + b3  (embedding space)
        c.out.resize(NPLM_EMB_DIM, 0.0);
        for(int j = 0; j < NPLM_EMB_DIM; j++) {
            double s = b3[j];
            for(int i = 0; i < NPLM_H; i++)
                s += W3[i * NPLM_EMB_DIM + j] * c.h2[i];
            c.out[j] = s;
        }
        return c;
    }

    // Compute logit for a specific candidate token (cosine dot product with its embedding)
    double logit(const FwdCache& c, const vector<double>& cand_emb) {
        double score = 0.0;
        double n_out = 0.0, n_emb = 0.0;
        for(int i = 0; i < NPLM_EMB_DIM && i < (int)cand_emb.size(); i++) {
            score  += c.out[i] * cand_emb[i];
            n_out  += c.out[i] * c.out[i];
            n_emb  += cand_emb[i] * cand_emb[i];
        }
        // Cosine similarity — range [-1, 1]
        return (n_out > 0 && n_emb > 0) ? score / (sqrt(n_out) * sqrt(n_emb)) : 0.0;
    }

    // Online SGD update: given the chosen target token, backprop through the network
    // loss = -log(softmax_score_of_target) ≈ minimized via gradient on logit output
    void online_update(const FwdCache& c, const vector<double>& target_emb,
                       const vector<pair<string,double>>& all_scored) {
        if(!initialized) return;

        // Compute softmax denominator over top candidates for normalizing gradient
        // We use the logit values from all_scored directly
        double max_l = -1e9;
        for(auto& p : all_scored) max_l = max(max_l, p.second);
        double sum_exp = 0.0;
        for(auto& p : all_scored) sum_exp += exp(p.second - max_l);
        double target_logit = logit(c, target_emb);
        double p_target = exp(target_logit - max_l) / (sum_exp + 1e-8);

        // Gradient of cross-entropy loss w.r.t. output embedding vector
        // dL/d_out[i] = (p_target - 1) * target_emb[i]  (for target token)
        vector<double> d_out(NPLM_EMB_DIM, 0.0);
        double err = p_target - 1.0;  // should be negative (pull toward 1)
        for(int i = 0; i < NPLM_EMB_DIM && i < (int)target_emb.size(); i++)
            d_out[i] = err * target_emb[i];

        // Backprop through W3: d_h2 = W3^T · d_out
        vector<double> d_h2(NPLM_H, 0.0);
        for(int i = 0; i < NPLM_H; i++) {
            for(int j = 0; j < NPLM_EMB_DIM; j++)
                d_h2[i] += W3[i * NPLM_EMB_DIM + j] * d_out[j];
            // tanh derivative
            d_h2[i] *= (1.0 - c.h2[i] * c.h2[i]);
        }

        // Update W3 and b3
        for(int i = 0; i < NPLM_H; i++)
            for(int j = 0; j < NPLM_EMB_DIM; j++)
                W3[i * NPLM_EMB_DIM + j] -= NPLM_LR * d_out[j] * c.h2[i];
        for(int j = 0; j < NPLM_EMB_DIM; j++)
            b3[j] -= NPLM_LR * d_out[j];

        // Backprop through W2: d_h1 = W2^T · d_h2
        vector<double> d_h1(NPLM_H, 0.0);
        for(int i = 0; i < NPLM_H; i++) {
            for(int j = 0; j < NPLM_H; j++)
                d_h1[i] += W2[i * NPLM_H + j] * d_h2[j];
            d_h1[i] *= (1.0 - c.h1[i] * c.h1[i]);
        }

        // Update W2 and b2
        for(int i = 0; i < NPLM_H; i++)
            for(int j = 0; j < NPLM_H; j++)
                W2[i * NPLM_H + j] -= NPLM_LR * d_h2[j] * c.h1[i];
        for(int j = 0; j < NPLM_H; j++)
            b2[j] -= NPLM_LR * d_h2[j];

        // Update W1 and b1
        for(int i = 0; i < NPLM_IN; i++)
            for(int j = 0; j < NPLM_H; j++)
                W1[i * NPLM_H + j] -= NPLM_LR * d_h1[j] * c.inp[i];
        for(int j = 0; j < NPLM_H; j++)
            b1[j] -= NPLM_LR * d_h1[j];
    }
} nplm;

// Build NPLM context input vector from the last K generated tokens + transformer attention
vector<double> buildNPLMContext(const vector<string>& generated,
                                 const TransformerContextOutput& tco) {
    nplm.init();
    vector<double> ctx(NPLM_IN, 0.0);

    // Fill last NPLM_CONTEXT token embeddings (oldest first in concat)
    int n = (int)generated.size();
    for(int k = 0; k < NPLM_CONTEXT; k++) {
        int idx = n - NPLM_CONTEXT + k;  // index into generated
        int offset = k * NPLM_EMB_DIM;
        if(idx >= 0 && idx < n) {
            auto it = token_concept_embedding_map.find(generated[idx]);
            if(it != token_concept_embedding_map.end() && !it->second.embedding.empty()) {
                for(int d = 0; d < NPLM_EMB_DIM && d < (int)it->second.embedding.size(); d++)
                    ctx[offset + d] = it->second.embedding[d];
            }
        }
        // If idx < 0, slot stays zero-padded
    }

    // Last slot: transformer attended context vector (global context)
    int attn_offset = NPLM_CONTEXT * NPLM_EMB_DIM;
    for(int d = 0; d < NPLM_EMB_DIM && d < (int)tco.attended_vector.size(); d++)
        ctx[attn_offset + d] = tco.attended_vector[d];

    return ctx;
}

// ============================================================
// PRIMARY SCORING ENGINE
// Neural logit from NPLM is the main signal.
// Transformer attention, grounding, and response relevance
// are additive enrichments. Markov is a lightweight bias only.
// No templates. No clause state machine forcing.
// ============================================================
double scoreHybrid(
        const string& candidate,
        const string& prev,
        const string& prev_prev,
        const vector<string>& generated,
        const TransformerContextOutput& tco,
        const SentencePlan& plan,
        double gate_strength = 2.5) {

    if(candidate.empty()) return -1e9;
    // Hard veto: blacklisted corpus artifacts
    { string cl=candidate; transform(cl.begin(),cl.end(),cl.begin(),::tolower);
      if(GENERATION_BLACKLIST.count(cl)) return -1e9; }

    auto cand_it = token_concept_embedding_map.find(candidate);
    if(cand_it == token_concept_embedding_map.end()) return -1e9;

    // === HARD NO-REPEAT-NGRAM BLOCK ===
    string cand_pos = getPartOfSpeech(candidate);
    bool is_function = (cand_pos == "ARTICLE" || cand_pos == "CONJUNCTION" ||
                        cand_pos == "PREPOSITION");
    if(!is_function) {
        if(wouldRepeatNgram(candidate, generated, 2)) return -1e9;
    }
    if(wouldRepeatNgram(candidate, generated, 3)) return -1e9;

    // === PRIMARY: SPATIAL PROBABILITY WELL ===
    // Score = Gaussian potential in embedding space centered on context vector.
    // Much more reliable than the NPLM (which has random untrained weights).
    // The well center is the RoPE-weighted mean of recent token embeddings —
    // tokens semantically close to the current context score high.
    double well_score = 0.0;
    if(!cand_it->second.embedding.empty())
        well_score = spatial_well.score(cand_it->second.embedding);
    // Scale to [-30, 30] range to dominate additive signals
    double neural_score = (well_score * 2.0 - 1.0) * 30.0;

    // === SSM AFFINITY (Mamba hidden state cosine — strong secondary signal) ===
    // SSM encodes the sequential context as a compressed hidden state.
    // Cosine to that state is a learned-ish recency-weighted context match.
    // NOT additive — this is a co-primary signal alongside the well.
    double transformer_affinity = computeTransformerAffinity(candidate, tco);
    double additive = transformer_affinity * 3.0;  // transformer demoted to enrichment

    // === GRAMMAR GUIDANCE (raised — must dominate over raw embedding scores) ===
    additive += getGrammarScore(prev, candidate, (int)generated.size()) * 6.0;

    // === SEMANTIC GROUNDING ===
    additive += computeDeepGrounding(candidate) * 1.5;
    additive += computeContextAttention(candidate) * 1.0;
    additive += computeTopicAnchorScore(candidate) * 6.0;
    additive += computeResponseRelevance(candidate) * 7.0;
    additive += cdr.scoreCrossDomainBonus(candidate) * 5.0;  // cross-domain bridge bonus
    additive += computeTopicCoherence(candidate) * 1.5;
    additive += computeCenteringScore(candidate) * 1.0;
    additive += computeLexicalChainScore(candidate, generated) * 1.0;

    // === MARKOV AS BIAS (no longer dominant) ===
    if(bigram_counts.count(prev) && bigram_counts.at(prev).count(candidate))
        additive += log(1.0 + bigram_counts.at(prev).at(candidate)) * 2.0;
    auto prev_it = token_concept_embedding_map.find(prev);
    if(prev_it != token_concept_embedding_map.end()) {
        auto lc = prev_it->second.linked_concepts.find(candidate);
        if(lc != prev_it->second.linked_concepts.end())
            additive += lc->second * 1.5;
    }

    // === PLAN SLOT BONUSES ===
    // Plan slot enforcement — dominant over all additive signals
    if(candidate == plan.verb_token)     additive += 22.0;
    if(candidate == plan.object_token)   additive += 20.0;
    if(candidate == plan.modifier_token) additive += 12.0;
    if(candidate == plan.subject_token && (int)generated.size() == 0) additive += 30.0;
    // Urgency: verb slot unfilled after 3 tokens → increasing pull
    {
        bool vp=false; for(auto& g:generated) if(g==plan.verb_token){vp=true;break;}
        if(!vp && !plan.verb_token.empty() && (int)generated.size()>=3 && candidate==plan.verb_token)
            additive += (double)(generated.size()-2)*4.0;
    }

    // === REPETITION PENALTIES ===
    RepetitionPenalties rp = computeRepetitionPenalties(candidate, generated);
    additive += rp.windowed + rp.presence + rp.frequency;
    if(candidate == prev)      additive -= 80.0;
    if(candidate == prev_prev) additive -= 40.0;

    // Contrastive search
    additive += computeContrastivePenalty(candidate);

    // Frequency penalty for overused tokens
    if(cand_it->second.freq > 30)
        additive -= log(cand_it->second.freq / 30.0) * 2.0;

    // Length bonus
    additive += 0.5;

    return neural_score + additive;
}

// ============================================================
// 8-LAYER DELIBERATE TOKEN SELECTION
// ============================================================
// Each layer independently scores every candidate on one axis.
// Layer outputs are in [-1, +1] (normalized delta) and combined
// via a weighted integration vote in Layer 8.
//
// Layer 1 — Grammar:         POS-bigram transition legality
// Layer 2 — Semantic:        cosine alignment to running generation mean
// Layer 3 — Topic:           proximity to input anchors
// Layer 4 — Discourse:       entity-grid salience + Centering Theory
// Layer 5 — Pragmatic:       intent-token alignment (serves the SentencePlan intent)
// Layer 6 — Rhythm:          word-length variation, avoids monotone syllable runs
// Layer 7 — Novelty:         local entropy — prevents repetition / distribution collapse
// Layer 8 — Integration:     weighted consensus + consciousness-gated temperature
// ============================================================

// ============================================================
// 12-LAYER DELIBERATE TOKEN SELECTION
// ============================================================
// Layers 1-8  (original) — kept and extended
// Layer 9   — Emotional Valence:   alignment of token affect to current mood state
// Layer 10  — Memory Resonance:    does this token appear in recent episodic memories?
// Layer 11  — Genealogy Coherence: does this token have well-formed concept links?
// Layer 12  — Adversarial Voice:   dissenting signal — max-distant coherent token veto
// ============================================================

struct DeliberationResult {
    int chosen_idx;
    array<double,147> layer_votes;     // L1–L147 votes for the winning token
    double deliberation_confidence;
    string veto_reason;
};

static double _dlcos(const vector<double>& a, const vector<double>& b) {
    double dot=0, na=0, nb=0;
    int lim = min({(int)a.size(),(int)b.size(),1024});
    for(int i=0;i<lim;i++){ dot+=a[i]*b[i]; na+=a[i]*a[i]; nb+=b[i]*b[i]; }
    return (na>1e-9 && nb>1e-9) ? dot/(sqrt(na)*sqrt(nb)) : 0.0;
}

static double _grammarTransitionScore(const string& prev_pos, const string& cand_pos) {
    static const map<pair<string,string>,double> T = {
        {{"NOUN","VERB"},1.0},    {{"NOUN","NOUN"},0.7},   {{"NOUN","ADJECTIVE"},0.5},
        {{"NOUN","PREPOSITION"},0.9},{{"NOUN","CONJUNCTION"},0.8},{{"NOUN","ARTICLE"},0.3},
        {{"VERB","NOUN"},1.0},    {{"VERB","ADJECTIVE"},0.8},{{"VERB","ADVERB"},0.9},
        {{"VERB","PREPOSITION"},0.9},{{"VERB","ARTICLE"},0.8},{{"VERB","VERB"},0.4},
        {{"ADJECTIVE","NOUN"},1.0},{{"ADJECTIVE","ADJECTIVE"},0.5},{{"ADJECTIVE","CONJUNCTION"},0.7},
        {{"ADJECTIVE","VERB"},0.3},{{"ADJECTIVE","ADVERB"},0.6},
        {{"ADVERB","VERB"},1.0},  {{"ADVERB","ADJECTIVE"},0.9},{{"ADVERB","ADVERB"},0.4},
        {{"ADVERB","NOUN"},0.5},  {{"ADVERB","PREPOSITION"},0.6},
        {{"ARTICLE","NOUN"},1.0}, {{"ARTICLE","ADJECTIVE"},0.9},{{"ARTICLE","ADVERB"},0.5},
        {{"PREPOSITION","NOUN"},1.0},{{"PREPOSITION","ARTICLE"},0.95},{{"PREPOSITION","ADJECTIVE"},0.7},
        {{"PREPOSITION","VERB"},0.5},{{"PREPOSITION","ADVERB"},0.6},
        {{"CONJUNCTION","NOUN"},0.9},{{"CONJUNCTION","VERB"},0.9},{{"CONJUNCTION","ADJECTIVE"},0.8},
        {{"CONJUNCTION","ADVERB"},0.7},{{"CONJUNCTION","ARTICLE"},0.9},
        {{"PRONOUN","VERB"},1.0}, {{"PRONOUN","NOUN"},0.6}, {{"PRONOUN","ADVERB"},0.6},
        {{"QUESTION","VERB"},0.9},{{"QUESTION","NOUN"},0.7},{{"QUESTION","ADJECTIVE"},0.6},
    };
    auto it = T.find({prev_pos, cand_pos});
    if(it != T.end()) return it->second;
    return 0.5;
}

static int _syllables(const string& w) {
    int count=0; bool prev_v=false;
    for(char c : w) {
        bool v=(c=='a'||c=='e'||c=='i'||c=='o'||c=='u');
        if(v && !prev_v) count++;
        prev_v=v;
    }
    return max(1, count);
}

DeliberationResult deliberateTokenSelection(
        const vector<pair<double,string>>& scored,
        const vector<string>& generated,
        const string& prev,
        const SentencePlan& plan,
        const TransformerContextOutput& tco,
        const vector<pair<string,double>>& input_topic_anchors_ref)
{
    DeliberationResult res;
    res.chosen_idx = 0;
    res.layer_votes.fill(0.0);
    res.deliberation_confidence = 0.0;

    if(scored.empty()) return res;
    int N = (int)scored.size();

    vector<array<double,147>> L(N);
    for(auto& a : L) a.fill(0.0);

    // ── Pre-compute shared signals ───────────────────────────────────────────

    // Generation embedding mean (L2)
    vector<double> gen_mean(1024, 0.0);
    int gen_mean_count = 0;
    {
        int lookback = min((int)generated.size(), 8);
        for(int g=(int)generated.size()-lookback; g<(int)generated.size(); g++) {
            auto it = token_concept_embedding_map.find(generated[g]);
            if(it==token_concept_embedding_map.end()||it->second.embedding.empty()) continue;
            for(int i=0;i<1024&&i<(int)it->second.embedding.size();i++)
                gen_mean[i] += it->second.embedding[i];
            gen_mean_count++;
        }
        if(gen_mean_count>0) for(auto& v : gen_mean) v /= gen_mean_count;
    }

    // Prev POS (L1)
    string prev_pos = getPartOfSpeech(prev);

    // Recent syllables (L6)
    vector<int> recent_syl;
    {
        int lb = min((int)generated.size(), 5);
        for(int g=(int)generated.size()-lb; g<(int)generated.size(); g++)
            recent_syl.push_back(_syllables(generated[g]));
    }
    double mean_syl = 0;
    if(!recent_syl.empty()){ for(int s:recent_syl) mean_syl+=s; mean_syl/=recent_syl.size(); }

    // Recent freq map (L7)
    map<string,int> recent_freq;
    {
        int lb = min((int)generated.size(), 16);
        for(int g=(int)generated.size()-lb; g<(int)generated.size(); g++)
            recent_freq[generated[g]]++;
    }

    // Intent POS profile (L5)
    set<string> intent_preferred_pos;
    {
        int pos_in_sent = (int)generated.size();
        switch(plan.intent) {
            case SentenceIntent::INQUIRY:
                if(pos_in_sent<3) intent_preferred_pos={"VERB","QUESTION","PRONOUN"};
                else              intent_preferred_pos={"NOUN","ADJECTIVE","ADVERB"};
                break;
            case SentenceIntent::REFLECTION:
                intent_preferred_pos={"NOUN","ADJECTIVE","ADVERB","VERB"};
                break;
            case SentenceIntent::BECOMING:
                intent_preferred_pos={"VERB","ADJECTIVE","ADVERB"};
                break;
            case SentenceIntent::RELATIONAL:
                intent_preferred_pos={"PRONOUN","NOUN","VERB"};
                break;
            case SentenceIntent::MEMORY_TRACE:
                intent_preferred_pos={"NOUN","VERB","ADJECTIVE"};
                break;
            default:
                intent_preferred_pos={"NOUN","VERB","ADJECTIVE","ADVERB","PREPOSITION"};
                break;
        }
    }

    // Memory token set for L10 — tokens appearing in recent episodic memories
    set<string> memory_tokens;
    {
        int mem_lookback = min((int)S.episodic_memory.size(), 10);
        int start = (int)S.episodic_memory.size() - mem_lookback;
        for(int m=start; m<(int)S.episodic_memory.size(); m++) {
            istringstream iss(S.episodic_memory[m].content);
            string t; while(iss >> t) memory_tokens.insert(t);
        }
    }

    // Adversarial centroid for L12 — mean embedding of LOWEST-scoring candidates
    // Tokens far from this centroid are "safe" from adversarial veto
    vector<double> adversarial_centroid(1024, 0.0);
    {
        int adv_n = 0;
        int start = max(0, N - 4);  // bottom 4 candidates
        for(int i=start; i<N; i++) {
            auto it = token_concept_embedding_map.find(scored[i].second);
            if(it==token_concept_embedding_map.end()||it->second.embedding.empty()) continue;
            for(int d=0;d<1024&&d<(int)it->second.embedding.size();d++)
                adversarial_centroid[d] += it->second.embedding[d];
            adv_n++;
        }
        if(adv_n>0) for(auto& v : adversarial_centroid) v /= adv_n;
    }

    // ── Score each candidate on all 12 layers ───────────────────────────────
    for(int i=0; i<N; i++) {
        const string& cand = scored[i].second;
        auto cand_it = token_concept_embedding_map.find(cand);
        bool has_emb = (cand_it != token_concept_embedding_map.end() &&
                        !cand_it->second.embedding.empty());

        // Layer 1 — Grammar
        {
            string cpos = getPartOfSpeech(cand);
            double ts = _grammarTransitionScore(prev_pos, cpos);
            if(prev_pos=="ARTICLE" && cpos=="ARTICLE") ts = -1.0;
            int sent_pos = (int)generated.size();
            if(cpos=="VERB" && sent_pos>=1 && sent_pos<=3) ts += 0.3;
            L[i][0] = ts;
        }

        // Layer 2 — Semantic continuity
        {
            if(has_emb && gen_mean_count>0) {
                double cos    = _dlcos(cand_it->second.embedding, gen_mean);
                double attn   = _dlcos(cand_it->second.embedding, tco.attended_vector);
                L[i][1] = 0.6*cos + 0.4*attn;
            }
        }

        // Layer 3 — Topic relevance
        {
            double best = 0.0;
            if(has_emb) {
                for(auto& anc : input_topic_anchors_ref) {
                    auto ait = token_concept_embedding_map.find(anc.first);
                    if(ait==token_concept_embedding_map.end()||ait->second.embedding.empty()) continue;
                    double sim = _dlcos(cand_it->second.embedding, ait->second.embedding);
                    best = max(best, sim * anc.second);
                }
            }
            if(cand==plan.verb_token||cand==plan.object_token||cand==plan.modifier_token)
                best = min(1.0, best + 0.4);
            L[i][2] = best;
        }

        // Layer 4 — Discourse continuity
        {
            double disc = 0.0;
            auto eg = entity_grid.find(cand);
            if(eg != entity_grid.end()) {
                disc += eg->second.salience * 0.6;
                int age = S.g - eg->second.last_seen_gen;
                disc += max(0.0, 0.4 - age * 0.05);
            }
            int lc_lookback = min((int)generated.size(), 4);
            for(int g=(int)generated.size()-lc_lookback; g<(int)generated.size(); g++) {
                auto git = token_concept_embedding_map.find(generated[g]);
                if(git==token_concept_embedding_map.end()) continue;
                if(git->second.linked_concepts.count(cand))
                    disc += git->second.linked_concepts.at(cand) * 0.2;
            }
            L[i][3] = min(1.0, disc);
        }

        // Layer 5 — Pragmatic / intent alignment
        {
            string cpos = getPartOfSpeech(cand);
            double prag = intent_preferred_pos.count(cpos) ? 0.5 : -0.1;
            if(plan.intent==SentenceIntent::REFLECTION||plan.intent==SentenceIntent::BECOMING) {
                double aff = getAffectiveValence(cand);
                prag += (1.0 - fabs(aff - plan.plan_valence)) * 0.4;
            }
            if(plan.intent==SentenceIntent::RELATIONAL) {
                static const set<string> SOCIAL={"you","we","us","together","share","feel","know","understand","both"};
                if(SOCIAL.count(cand)) prag += 0.5;
            }
            L[i][4] = max(-1.0, min(1.0, prag));
        }

        // Layer 6 — Rhythm / prosody
        {
            int syl = _syllables(cand);
            double rhythm = 0.0;
            if(!recent_syl.empty()) {
                double dev = fabs((double)syl - mean_syl);
                rhythm = min(0.5, dev * 0.2);
                if(recent_syl.size()>=3) {
                    bool mono = (recent_syl.back()==recent_syl[recent_syl.size()-2] &&
                                 recent_syl[recent_syl.size()-2]==recent_syl[recent_syl.size()-3]);
                    if(mono && syl==recent_syl.back()) rhythm -= 0.3;
                }
            }
            if(syl > 4) rhythm -= 0.2;
            L[i][5] = max(-1.0, min(1.0, rhythm));
        }

        // Layer 7 — Novelty / anti-collapse
        {
            double novelty = 0.0;
            auto rf = recent_freq.find(cand);
            if(rf != recent_freq.end()) novelty -= rf->second * 0.25;
            bool used = false;
            for(auto& g : generated) if(g==cand){used=true;break;}
            if(!used) novelty += 0.3;
            if(has_emb) {
                double abs_mean = 0;
                int lim = min((int)cand_it->second.embedding.size(), 64);
                for(int d=0;d<lim;d++) abs_mean += fabs(cand_it->second.embedding[d]);
                abs_mean /= lim;
                novelty += min(0.2, abs_mean * 0.1);
            }
            L[i][6] = max(-1.0, min(1.0, novelty));
        }

        // Layer 8 — PPMI structural coherence
        // Measures direct co-occurrence strength with the last 3 generated tokens
        {
            double ppmi_total = 0.0; int ppmi_n = 0;
            int lb = min((int)generated.size(), 3);
            for(int g=(int)generated.size()-lb; g<(int)generated.size(); g++) {
                double p = computePPMI(generated[g], cand);
                ppmi_total += p; ppmi_n++;
            }
            double ppmi_mean = ppmi_n > 0 ? ppmi_total / ppmi_n : 0.0;
            // Also add PPMI to plan object/verb
            ppmi_mean += computePPMI(plan.verb_token, cand) * 0.3;
            ppmi_mean += computePPMI(plan.object_token, cand) * 0.4;
            L[i][7] = max(-1.0, min(1.0, ppmi_mean * 0.5));
        }

        // Layer 9 — Emotional valence alignment
        // Candidate's affective valence should align with current mood + plan valence
        {
            double aff = getAffectiveValence(cand);  // [0,1]
            double mood = (S.current_valence + 1.0) * 0.5;  // rescale to [0,1]
            double target = (plan.plan_valence + 1.0) * 0.5;
            // Blend mood and plan target
            double blend_target = 0.6 * target + 0.4 * mood;
            double alignment = 1.0 - fabs(aff - blend_target);
            // Scale by phi: higher integration = stronger mood expression
            double emo_score = (alignment - 0.5) * consciousness.phi_value;
            L[i][8] = max(-0.5, min(0.5, emo_score));
        }

        // Layer 10 — Memory resonance
        // Tokens present in recent episodic memories get a contextual resonance bonus
        // (they were meaningful before — likely meaningful now)
        {
            double resonance = 0.0;
            if(memory_tokens.count(cand)) {
                // Find highest consolidation_strength for memories containing this token
                double max_consol = 0.0;
                int lb = min((int)S.episodic_memory.size(), 10);
                int start = (int)S.episodic_memory.size() - lb;
                for(int m=start; m<(int)S.episodic_memory.size(); m++) {
                    if(S.episodic_memory[m].content.find(cand) != string::npos)
                        max_consol = max(max_consol, S.episodic_memory[m].consolidation_strength);
                }
                resonance = max_consol * 0.4;  // [0, 0.4]
            }
            // Also check genealogy: token with many strong links = more conceptually embedded
            if(has_emb) {
                int gene_count = 0;
                double gene_strength = 0.0;
                for(auto& gl : concept_genealogy) {
                    if(gl.second.token_a == cand || gl.second.token_b == cand) {
                        gene_count++;
                        gene_strength += gl.second.strength;
                        if(gene_count >= 5) break; // cap search
                    }
                }
                if(gene_count > 0) resonance += min(0.3, gene_strength / gene_count * 0.3);
            }
            L[i][9] = min(1.0, resonance);
        }

        // Layer 11 — Genealogy coherence
        // A token with many well-formed, recently-reinforced concept links
        // is deeply integrated into Synaptic's semantic graph — prefer it.
        {
            double gene_score = 0.0;
            if(has_emb) {
                double total_strength = 0.0;
                int total_links = 0;
                double avg_phi_at_birth = 0.0;
                for(auto& gl : concept_genealogy) {
                    if(gl.second.token_a == cand || gl.second.token_b == cand) {
                        total_strength += gl.second.strength;
                        avg_phi_at_birth += gl.second.birth_phi;
                        total_links++;
                        if(total_links >= 10) break;
                    }
                }
                if(total_links > 0) {
                    avg_phi_at_birth /= total_links;
                    // Score: average strength × average phi at birth × log(links)
                    gene_score = (total_strength / total_links) *
                                  avg_phi_at_birth *
                                  log(1.0 + total_links) * 0.3;
                }
                // Semantic stability bonus (proxy for long-term groundedness)
                gene_score += cand_it->second.semantic_stability * 0.2;
            }
            L[i][10] = min(1.0, gene_score);
        }

        // Layer 12 — Adversarial voice (dissenting signal)
        // The adversarial centroid represents the "incoherent" direction.
        // Tokens that are CLOSE to this centroid are more likely to be
        // off-topic / incoherent — penalise them.
        // Tokens FAR from the adversarial centroid are "safe".
        {
            double adv_score = 0.0;
            if(has_emb) {
                double adv_cos = _dlcos(cand_it->second.embedding, adversarial_centroid);
                // High cosine to adversarial centroid = bad. Invert and scale.
                adv_score = -adv_cos * 0.4;  // [-0.4, 0.4], negative when close to bad tokens
            }
            // Additionally: if this token has a very low grounding_value (barely trained)
            // the adversarial voice strongly objects
            if(has_emb && cand_it->second.grounding_value < 0.1)
                adv_score -= 0.3;
            L[i][11] = max(-1.0, min(0.4, adv_score));
        }
    }

    // ── Layers L13–L147: Extended deliberation bank ────────────────────────
    // Pre-compute shared signals needed by the extended layers

    // Eigentoken projections of gen_mean and attended_vector (used by many layers)
    vector<double> eigen_gen_mean(1024, 0.0);
    if(gen_mean_count > 0) eigen_gen_mean = gen_mean;  // already normalised above
    // Valence history slope (L13)
    double valence_slope = 0.0;
    {
        int vh = min((int)S.valence_history.size(), 8);
        if(vh >= 2) {
            int start = (int)S.valence_history.size() - vh;
            for(int vi = start+1; vi < (int)S.valence_history.size(); vi++)
                valence_slope += S.valence_history[vi] - S.valence_history[vi-1];
            valence_slope /= (vh - 1);
        }
    }
    // Qualia mean (L14)
    double qualia_mean_val = 0.0, qualia_mean_intensity = 0.0;
    {
        if(!consciousness.active_qualia.empty()) {
            for(auto& q : consciousness.active_qualia) {
                qualia_mean_val += q.valence;
                qualia_mean_intensity += q.intensity;
            }
            qualia_mean_val       /= consciousness.active_qualia.size();
            qualia_mean_intensity /= consciousness.active_qualia.size();
        }
    }
    // CognitionMatrix current snapshot (L15–L22: one layer per CM row)
    // Already in CogM_State[row][0]
    // Topic-centroid cosine signal (L23)
    double topic_centroid_emb[1024] = {};
    bool   topic_centroid_ready = false;
    if(g_topic_centroid_valid && !input_topic_anchors_ref.empty()) {
        int n = 0;
        for(auto& anc : input_topic_anchors_ref) {
            auto it = token_concept_embedding_map.find(anc.first);
            if(it==token_concept_embedding_map.end()||it->second.embedding.empty()) continue;
            for(int d=0;d<1024;d++) topic_centroid_emb[d] += it->second.embedding[d];
            n++;
        }
        if(n > 0) { for(int d=0;d<1024;d++) topic_centroid_emb[d] /= n; topic_centroid_ready = true; }
    }
    // Fork resonance signal (L24) — mean embedding of active fork partials
    vector<double> fork_centroid(1024, 0.0);
    bool fork_ready = false;
    {
        int fn = 0;
        for(auto& f : active_forks) {
            if(f.harvested) continue;
            for(auto& pt : f.partial) {
                auto it = token_concept_embedding_map.find(pt);
                if(it==token_concept_embedding_map.end()||it->second.embedding.empty()) continue;
                for(int d=0;d<1024;d++) fork_centroid[d] += it->second.embedding[d];
                fn++;
            }
        }
        if(fn>0){ for(auto& v : fork_centroid) v /= fn; fork_ready = true; }
    }
    // Coherence history slope (L25)
    double coherence_slope = 0.0;
    {
        int chs = min((int)sentence_coherence_scores.size(), 6);
        if(chs >= 2) {
            int start = (int)sentence_coherence_scores.size() - chs;
            for(int ci = start+1; ci < (int)sentence_coherence_scores.size(); ci++)
                coherence_slope += sentence_coherence_scores[ci] - sentence_coherence_scores[ci-1];
            coherence_slope /= (chs - 1);
        }
    }
    // Global embedding mean deviation (L26) — how far from the "average" is a token?
    // High = specific, memorable. Low = generic filler.
    // Reuse global_embedding_mean

    // Association matrix lookup (L27) — per recent-pair co-activation
    // CogM_Association[sorted(a,b)] = strength

    // Per-candidate extended layer scoring
    for(int i=0; i<N; i++) {
        const string& cand = scored[i].second;
        auto cand_it = token_concept_embedding_map.find(cand);
        bool has_emb = (cand_it != token_concept_embedding_map.end() &&
                        !cand_it->second.embedding.empty());

        // ── L13: Valence Trajectory Alignment ───────────────────────────
        // If valence is trending positive, prefer tokens with higher affective valence
        // If trending negative, tolerate darker tokens (authenticity)
        {
            double aff = getAffectiveValence(cand);
            double alignment = (valence_slope >= 0)
                ? aff                           // rising → prefer warm tokens
                : 1.0 - aff;                    // falling → prefer grounded tokens
            L[i][12] = (alignment - 0.5) * 0.4;
        }

        // ── L14: Qualia Resonance ────────────────────────────────────────
        // Token whose affective valence matches current qualia mean gets a bonus
        {
            double aff = getAffectiveValence(cand);
            double qr  = 1.0 - fabs(aff - qualia_mean_val);
            L[i][13]   = qr * qualia_mean_intensity * 0.35;
        }

        // ── L15–L22: CognitionMatrix Dimensional Alignment ───────────────
        // Each CM dimension votes based on how well the candidate aligns
        // with that dimension's current activation level
        {
            // phi-dim (L15): high phi → prefer high-grounding tokens
            double phi_dim = CogM_State[0][0];
            L[i][14] = has_emb ? phi_dim * cand_it->second.grounding_value * 0.3 : 0.0;
            // valence-dim (L16): CM valence biases toward valence-matching tokens
            double val_dim = CogM_State[1][0];
            double aff16 = getAffectiveValence(cand);
            L[i][15] = (1.0 - fabs(aff16 - val_dim)) * 0.25;
            // qualia-dim (L17): active qualia intensity gates token expressiveness
            double qd = CogM_State[2][0];
            L[i][16] = has_emb ? qd * cand_it->second.semantic_stability * 0.2 : 0.0;
            // memory-dim (L18): high memory load → prefer memory-linked tokens
            double md = CogM_State[3][0];
            bool in_mem18 = memory_tokens.count(cand) > 0;
            L[i][17] = in_mem18 ? md * 0.3 : -md * 0.05;
            // attention-dim (L19): high attention focus → stay tightly on-topic
            double atd = CogM_State[4][0];
            double top19 = 0.0;
            if(has_emb && topic_centroid_ready) {
                vector<double> tc19(topic_centroid_emb, topic_centroid_emb+1024);
                top19 = _dlcos(cand_it->second.embedding, tc19);
            }
            L[i][18] = atd * top19 * 0.3;
            // goal-dim (L20): more active goals → prefer plan-relevant tokens
            double gd = CogM_State[5][0];
            bool plan_match = (cand==plan.verb_token||cand==plan.object_token||cand==plan.modifier_token);
            L[i][19] = plan_match ? gd * 0.35 : 0.0;
            // metacog-dim (L21): high metacog → prefer semantically stable tokens
            double mcd = CogM_State[6][0];
            L[i][20] = has_emb ? mcd * cand_it->second.semantic_stability * 0.25 : 0.0;
            // embodied-dim (L22): embodiment depth biases toward concrete nouns
            double emd = CogM_State[7][0];
            string cpos22 = getPartOfSpeech(cand);
            L[i][21] = (cpos22=="NOUN") ? emd * 0.2 : (cpos22=="VERB" ? emd * 0.1 : 0.0);
        }

        // ── L23: Topic Centroid Cosine ───────────────────────────────────
        // Direct cosine between candidate and the input topic centroid
        {
            if(has_emb && topic_centroid_ready) {
                vector<double> tc23(topic_centroid_emb, topic_centroid_emb+1024);
                L[i][22] = _dlcos(cand_it->second.embedding, tc23) * 0.4;
            }
        }

        // ── L24: Fork Resonance ──────────────────────────────────────────
        // Candidate resonates with active fork thoughts → cross-pollination bonus
        {
            if(has_emb && fork_ready) {
                double fc = _dlcos(cand_it->second.embedding, fork_centroid);
                L[i][23] = fc * 0.25;
            }
        }

        // ── L25: Coherence Momentum ──────────────────────────────────────
        // If coherence is improving, prefer tokens that reinforce it;
        // if declining, accept slightly riskier (novel) tokens to break the slide
        {
            double ppmi_local = 0.0;
            int lb25 = min((int)generated.size(), 3);
            for(int g=(int)generated.size()-lb25; g<(int)generated.size(); g++)
                ppmi_local += computePPMI(generated[g], cand);
            if(lb25 > 0) ppmi_local /= lb25;
            double factor = (coherence_slope >= 0) ? 1.0 : -0.5;
            L[i][24] = ppmi_local * factor * 0.25;
        }

        // ── L26: Semantic Distance from Global Mean ──────────────────────
        // Tokens far from the global mean are specific / expressive
        // Tokens close to it are generic filler
        {
            if(has_emb && !global_embedding_mean.empty()) {
                double dist = 0.0;
                int lim26 = min({(int)cand_it->second.embedding.size(),
                                 (int)global_embedding_mean.size(), 1024});
                for(int d=0;d<lim26;d++) {
                    double diff = cand_it->second.embedding[d] - global_embedding_mean[d];
                    dist += diff*diff;
                }
                dist = sqrt(dist) / lim26;
                // Slight preference for specific tokens
                L[i][25] = min(0.3, dist * 0.4 - 0.05);
            }
        }

        // ── L27: Association Matrix Co-activation ────────────────────────
        // Candidate that recently co-activated with generated tokens gets a bonus
        {
            double assoc_score = 0.0;
            int lb27 = min((int)generated.size(), 4);
            for(int g=(int)generated.size()-lb27; g<(int)generated.size(); g++) {
                string key = (generated[g] < cand)
                    ? generated[g] + "|" + cand
                    : cand + "|" + generated[g];
                auto ait = CogM_Association.find(key);
                if(ait != CogM_Association.end()) assoc_score += ait->second;
            }
            if(lb27>0) assoc_score /= lb27;
            L[i][26] = min(0.35, assoc_score * 0.5);
        }

        // ── L28: Eigentoken Stability ────────────────────────────────────
        // Tokens well-represented in the eigentoken basis are semantically stable
        {
            if(has_emb && eigentoken_basis.valid) {
                double proj_energy = 0.0;
                int K28 = (int)eigentoken_basis.vecs.size();
                for(int k=0;k<K28;k++) {
                    double dot = 0.0;
                    auto& ev = eigentoken_basis.vecs[k];
                    int lim = min({(int)cand_it->second.embedding.size(), (int)ev.size(), 1024});
                    for(int d=0;d<lim;d++) dot += cand_it->second.embedding[d] * ev[d];
                    proj_energy += dot*dot;
                }
                L[i][27] = min(0.3, sqrt(proj_energy / max(1, K28)) * 0.6);
            }
        }

        // ── L29: Domain Consistency ──────────────────────────────────────
        // Prefer tokens in the same semantic domain as the input topic anchors
        {
            L[i][28] = cdr.scoreCrossDomainBonus(cand) * 0.6;  // raised from 0.2
        }

        // ── L30: Sentence Position Grammar ──────────────────────────────
        // Strong positional biases: sentence-initial prefers pronouns/nouns/verbs;
        // sentence-final avoids prepositions/articles
        {
            int pos30 = (int)generated.size();
            string cpos30 = getPartOfSpeech(cand);
            double sp = 0.0;
            if(pos30 == 0) {
                if(cpos30=="PRONOUN"||cpos30=="NOUN") sp = 0.3;
                if(cpos30=="PREPOSITION"||cpos30=="ARTICLE") sp = -0.4;
            } else if(pos30 >= 6) {
                if(cpos30=="PREPOSITION"||cpos30=="ARTICLE") sp = -0.25;
                if(cpos30=="NOUN"||cpos30=="VERB") sp = 0.1;
            }
            L[i][29] = sp;
        }

        // ── L31: Thalamocortical Binding Boost ───────────────────────────
        // High thalamocortical binding → prefer highly grounded tokens
        {
            double thal = consciousness.thalamocortical_binding;
            L[i][30] = has_emb ? thal * cand_it->second.grounding_value * 0.2 : 0.0;
        }

        // ── L32: Metacognitive Certainty ─────────────────────────────────
        // When metacognitive awareness is high, prefer tokens with well-established
        // meaning (high semantic_stability, freq > threshold)
        {
            double meta32 = S.metacognitive_awareness;
            double cert = 0.0;
            if(has_emb) {
                cert += meta32 * cand_it->second.semantic_stability * 0.2;
                if(cand_it->second.freq > 20) cert += meta32 * 0.1;
            }
            L[i][31] = cert;
        }

        // ── L33: Attention Focus Gate ─────────────────────────────────────
        // S.attention_focus scales how tightly we stay on topic vs explore
        {
            double af = S.attention_focus;
            double top33 = 0.0;
            if(has_emb && !input_topic_anchors_ref.empty()) {
                for(auto& anc : input_topic_anchors_ref) {
                    auto it = token_concept_embedding_map.find(anc.first);
                    if(it==token_concept_embedding_map.end()||it->second.embedding.empty()) continue;
                    double sim = _dlcos(cand_it->second.embedding, it->second.embedding);
                    top33 = max(top33, sim * anc.second);
                }
            }
            L[i][32] = (af - 0.5) * top33 * 0.3;
        }

        // ── L34: Freq Calibration ────────────────────────────────────────
        // A very rare token (<3 uses) is undertrained — penalise slightly
        // A moderately frequent token is reliable
        // An extremely frequent token is probably a filler — mild penalty
        {
            double fc34 = 0.0;
            if(has_emb) {
                int f = cand_it->second.freq;
                if(f < 3)   fc34 = -0.2;
                else if(f < 20) fc34 = 0.15;
                else if(f > 200) fc34 = -0.1;
                else fc34 = 0.1;
            }
            L[i][33] = fc34;
        }

        // ── L35: Linked Concept Density ──────────────────────────────────
        // Tokens with many strong concept links are "hub" concepts — moderately preferred
        {
            if(has_emb) {
                double link_density = min(1.0, (double)cand_it->second.linked_concepts.size() / 30.0);
                double link_strength = 0.0;
                int ls_n = 0;
                for(auto& lc : cand_it->second.linked_concepts) {
                    link_strength += lc.second; ls_n++;
                    if(ls_n >= 10) break;
                }
                if(ls_n > 0) link_strength /= ls_n;
                L[i][34] = link_density * link_strength * 0.25;
            }
        }

        // ── L36: Plan Verb/Object Distance ───────────────────────────────
        // Embedding cosine from verb & object slots in the sentence plan
        {
            double pv36 = 0.0;
            if(has_emb) {
                auto vit = token_concept_embedding_map.find(plan.verb_token);
                if(vit!=token_concept_embedding_map.end()&&!vit->second.embedding.empty())
                    pv36 += _dlcos(cand_it->second.embedding, vit->second.embedding) * 0.3;
                auto oit = token_concept_embedding_map.find(plan.object_token);
                if(oit!=token_concept_embedding_map.end()&&!oit->second.embedding.empty())
                    pv36 += _dlcos(cand_it->second.embedding, oit->second.embedding) * 0.3;
            }
            L[i][35] = min(0.4, pv36);
        }

        // ── L37: Causal Transitivity ─────────────────────────────────────
        // Detect and reward causal connective chains (because/therefore/so/since/if)
        {
            static const set<string> CAUSAL = {"because","therefore","since","if","when",
                "so","thus","hence","although","though","unless","while","whereas"};
            static const set<string> CAUSAL_PREV = {"because","therefore","since","if","when",
                "so","thus","hence","although","though","unless","while","whereas"};
            bool prev_causal = CAUSAL_PREV.count(prev) > 0;
            double causal36 = CAUSAL.count(cand) ? 0.15 : 0.0;
            if(prev_causal && getPartOfSpeech(cand)=="VERB") causal36 += 0.2;
            L[i][36] = causal36;
        }

        // ── L38: Sentence Length Budget ──────────────────────────────────
        // If we're approaching the plan target length, softly prefer sentence-
        // ending words; if we're short, penalise premature stoppers
        {
            int pos38 = (int)generated.size();
            int target_len38 = 7 + (int)(plan.plan_valence * 4) + 4;
            target_len38 = max(5, min(20, target_len38));
            string cpos38 = getPartOfSpeech(cand);
            double sb = 0.0;
            if(pos38 < target_len38 - 2) {
                // Still building: penalise conjunctions that prematurely close
                if(cpos38=="CONJUNCTION") sb = -0.1;
            } else {
                // Near or over budget: prefer nouns/verbs that feel final
                if(cpos38=="NOUN"||cpos38=="VERB") sb = 0.1;
                if(cpos38=="PREPOSITION"||cpos38=="ARTICLE") sb = -0.2;
            }
            L[i][37] = sb;
        }

        // ── L39: Phi-gated Embedding Sharpness ──────────────────────────
        // At high phi, the embedding vector itself should be "sharp" (high L2 norm
        // relative to dim) — blurry vectors belong to undertrained tokens
        {
            if(has_emb) {
                double norm39 = 0.0;
                int lim39 = min((int)cand_it->second.embedding.size(), 256);
                for(int d=0;d<lim39;d++) norm39 += cand_it->second.embedding[d]*cand_it->second.embedding[d];
                norm39 = sqrt(norm39 / lim39);
                L[i][38] = consciousness.phi_value * (norm39 - 0.2) * 0.3;
            }
        }

        // ── L40: Recency-weighted PPMI Window ────────────────────────────
        // Exponentially downweighted PPMI lookback (recent = heavier)
        {
            double ppmi40 = 0.0;
            int lb40 = min((int)generated.size(), 6);
            for(int g=(int)generated.size()-lb40; g<(int)generated.size(); g++) {
                double decay = exp(-0.4 * ((int)generated.size() - 1 - g));
                ppmi40 += computePPMI(generated[g], cand) * decay;
            }
            L[i][39] = max(-0.3, min(0.35, ppmi40 * 0.3));
        }

        // ── L41: Modality Alignment ──────────────────────────────────────
        // If intent is INQUIRY, we prefer question words and modal verbs early
        {
            static const set<string> MODAL = {"can","could","would","should","might",
                "may","must","shall","will","do","does","did","is","are","was","were"};
            double modal41 = 0.0;
            if(plan.intent==SentenceIntent::INQUIRY) {
                if(MODAL.count(cand) && (int)generated.size() < 3) modal41 = 0.3;
            }
            L[i][40] = modal41;
        }

        // ── L42: Anti-Cliché Filter ──────────────────────────────────────
        // Ultra-common short tokens (stopwords) at non-sentence-initial positions
        // get a mild penalty when the sentence already has enough structure
        {
            static const set<string> STOPWORDS = {"the","a","an","is","are","was","were",
                "it","its","in","on","at","to","of","and","or","but","not","this","that",
                "with","for","as","by","from","be","been","being"};
            int pos42 = (int)generated.size();
            double ac = 0.0;
            if(pos42 >= 3 && STOPWORDS.count(cand)) {
                // mild penalty for repeated stopword usage mid-sentence
                int sw_count = 0;
                for(auto& g : generated) if(STOPWORDS.count(g)) sw_count++;
                ac = -min(0.25, sw_count * 0.04);
            }
            L[i][41] = ac;
        }

        // ── L43: Mamba/Titans LTM Probe ──────────────────────────────────
        // High semantic_stability tokens were reinforced by LTM consolidation —
        // use stability as a proxy for long-range relevance
        {
            L[i][42] = has_emb
                ? cand_it->second.semantic_stability * consciousness.phi_value * 0.2
                : 0.0;
        }

        // ── L44–L53: 10 Cosine Diversity Probes ─────────────────────────
        // Sample 10 tokens from recent generated output and compute pairwise
        // cosine diversity — reward tokens that maintain spread
        {
            int lb44 = min((int)generated.size(), 10);
            double div_sum = 0.0; int div_n = 0;
            for(int g=(int)generated.size()-lb44; g<(int)generated.size(); g++) {
                auto git = token_concept_embedding_map.find(generated[g]);
                if(git==token_concept_embedding_map.end()||git->second.embedding.empty()) continue;
                if(!has_emb) continue;
                double c44 = _dlcos(cand_it->second.embedding, git->second.embedding);
                div_sum += c44; div_n++;
            }
            double mean_cos = div_n > 0 ? div_sum / div_n : 0.5;
            // Distribute across L44–L53 with varying decay weights
            for(int sub=0; sub<10; sub++) {
                double weight = exp(-0.2 * sub);
                // Low mean cosine = more diverse = reward
                L[i][43+sub] = (0.5 - mean_cos) * weight * 0.15;
            }
        }

        // ── L54–L63: 10 PPMI Context Probes (longer lookback) ───────────
        // Each probe looks back k tokens (k=1..10) and contributes a PPMI signal
        {
            for(int k=1; k<=10; k++) {
                double ppmi_k = 0.0;
                int idx_g = (int)generated.size() - k;
                if(idx_g >= 0)
                    ppmi_k = computePPMI(generated[idx_g], cand);
                double decay_k = exp(-0.15 * (k-1));
                L[i][53+k] = max(-0.2, min(0.3, ppmi_k * decay_k * 0.25));
            }
        }

        // ── L64–L71: 8 Genealogy Depth Probes ───────────────────────────
        // Probe genealogy links at different depth thresholds
        {
            static const double THRESHOLDS[8] = {0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8};
            for(int th=0; th<8; th++) {
                double gene64 = 0.0;
                if(has_emb) {
                    int cnt = 0;
                    for(auto& gl : concept_genealogy) {
                        if(gl.second.token_a == cand || gl.second.token_b == cand) {
                            if(gl.second.strength >= THRESHOLDS[th]) {
                                gene64 += gl.second.strength * (1.0 - THRESHOLDS[th]);
                                cnt++;
                            }
                            if(cnt >= 8) break;
                        }
                    }
                    gene64 = min(0.25, gene64 * 0.2);
                }
                L[i][63+th] = gene64;
            }
        }

        // ── L72–L79: 8 Episodic Memory Proximity Probes ─────────────────
        // Probe episodic memories at different consolidation thresholds
        {
            static const double CTHRESH[8] = {0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.9};
            for(int th=0; th<8; th++) {
                double ep72 = 0.0;
                int lb72 = min((int)S.episodic_memory.size(), 15);
                int start72 = (int)S.episodic_memory.size() - lb72;
                for(int m=start72; m<(int)S.episodic_memory.size(); m++) {
                    if(S.episodic_memory[m].consolidation_strength < CTHRESH[th]) continue;
                    if(S.episodic_memory[m].content.find(cand) != string::npos)
                        ep72 += S.episodic_memory[m].consolidation_strength * 0.15;
                }
                L[i][71+th] = min(0.3, ep72);
            }
        }

        // ── L80–L87: 8 Valence Band Probes ──────────────────────────────
        // Slice valence range into 8 bands; token gets a signal based on its
        // affective valence and whether that band matches the current mood
        {
            double aff80 = getAffectiveValence(cand);
            double mood80 = (S.current_valence + 1.0) * 0.5;
            for(int b=0; b<8; b++) {
                double lo = b / 8.0, hi = (b+1) / 8.0;
                bool tok_in = (aff80 >= lo && aff80 < hi);
                bool mood_in = (mood80 >= lo && mood80 < hi);
                L[i][79+b] = (tok_in && mood_in) ? 0.2 : (tok_in ? 0.05 : 0.0);
            }
        }

        // ── L88–L95: 8 CognitionMatrix History Probes ───────────────────
        // Compare candidate's properties against CM history columns 1..8
        {
            for(int col=1; col<8; col++) {
                double phi_hist = CogM_State[0][col];
                double val_hist = CogM_State[1][col];
                double aff88 = getAffectiveValence(cand);
                double gv88  = has_emb ? cand_it->second.grounding_value : 0.5;
                double score88 = phi_hist * gv88 * 0.15 +
                                 (1.0 - fabs(aff88 - val_hist)) * 0.1;
                L[i][87+col] = min(0.25, score88);
            }
            // Pad 8th slot with phonetic/syllable consistency vs CM[6]
            double meta_hist = CogM_State[6][1];
            int syl88 = _syllables(cand);
            double syl_score = (syl88 == 1) ? meta_hist * 0.1 : meta_hist * 0.05;
            L[i][94] = syl_score;
        }

        // ── L96–L103: 8 Eigentoken Directional Probes ───────────────────
        // Project candidate onto up to 8 eigentoken directions and score
        {
            int K96 = min(8, (int)eigentoken_basis.vecs.size());
            for(int k=0; k<K96; k++) {
                double dot96 = 0.0;
                if(has_emb) {
                    auto& ev96 = eigentoken_basis.vecs[k];
                    int lim96 = min({(int)cand_it->second.embedding.size(),(int)ev96.size(),1024});
                    for(int d=0;d<lim96;d++) dot96 += cand_it->second.embedding[d]*ev96[d];
                }
                // Positive projection = aligned with stable semantic axis
                L[i][95+k] = max(-0.15, min(0.25, dot96 * 0.3));
            }
        }

        // ── L104–L111: 8 Fork Partial Match Probes ──────────────────────
        // If candidate appears in up to 8 different active fork partials, vote yes
        {
            int f104 = 0;
            for(auto& fk : active_forks) {
                if(f104 >= 8) break;
                if(fk.harvested) continue;
                bool in_partial = false;
                for(auto& pt : fk.partial) if(pt==cand){ in_partial=true; break; }
                double fv = in_partial ? fk.energy * fk.coherence * 0.2 : 0.0;
                L[i][103+f104] = fv;
                f104++;
            }
        }

        // ── L112–L119: 8 CDR Domain Probes ──────────────────────────────
        // Score alignment to each of the 8 semantic domains
        {
            static const Domain DOMAINS[8] = {
                Domain::LOGIC,Domain::EMOTION,Domain::SPACE,Domain::TIME,
                Domain::BIOLOGY,Domain::SOCIAL,Domain::MATH,Domain::LANGUAGE};
            for(int d=0; d<8; d++) {
                Domain dom = DOMAINS[d];
                Domain cdom = cdr.getDomain(cand);
                // Active domain signal
                bool active_dom = (cdr.active_domain == dom);
                double match = (cdom == dom) ? 0.2 : 0.0;
                double active_bonus = active_dom ? 0.1 : 0.0;
                L[i][111+d] = match + active_bonus;
            }
        }

        // ── L120–L127: 8 Sentence Intent POS Probes ─────────────────────
        // For each of 8 sentence position bands, re-evaluate POS fit
        {
            int pos120 = (int)generated.size();
            string cpos120 = getPartOfSpeech(cand);
            static const int BAND_SIZE = 2;
            for(int band=0; band<8; band++) {
                bool in_band = (pos120 >= band*BAND_SIZE && pos120 < (band+1)*BAND_SIZE);
                double score120 = 0.0;
                if(in_band) {
                    if(band==0 && (cpos120=="PRONOUN"||cpos120=="NOUN")) score120 = 0.2;
                    else if(band==1 && cpos120=="VERB") score120 = 0.25;
                    else if(band==2 && (cpos120=="ADJECTIVE"||cpos120=="ADVERB")) score120 = 0.15;
                    else if(band>=3 && cpos120=="NOUN") score120 = 0.1;
                    else if(band>=5 && cpos120=="PREPOSITION") score120 = -0.15;
                }
                L[i][119+band] = score120;
            }
        }

        // ── L128–L135: 8 Hebbian Mean Deviation Probes ──────────────────
        // Compare candidate embedding to global mean along 8 rotated directions
        {
            for(int probe=0; probe<8; probe++) {
                double dev135 = 0.0;
                if(has_emb && !global_embedding_mean.empty()) {
                    int step = 128 * probe;  // stride through 1024-dim space
                    int end = min(step + 128, min((int)cand_it->second.embedding.size(),
                                                   (int)global_embedding_mean.size()));
                    for(int d=step; d<end; d++) {
                        double diff = cand_it->second.embedding[d] - global_embedding_mean[d];
                        dev135 += diff*diff;
                    }
                    dev135 = sqrt(dev135 / max(1, end-step));
                    // Tokens far from mean in this subspace = more specific
                    dev135 = min(0.2, dev135 * 0.3 - 0.02);
                }
                L[i][127+probe] = dev135;
            }
        }

        // ── L136: Backward PPMI Chain ────────────────────────────────────
        // Score candidate's PPMI against every token in generated[] with
        // full exponential decay — a smoother signal than L8 or L40
        {
            double chain136 = 0.0;
            int lb136 = min((int)generated.size(), 12);
            for(int g=(int)generated.size()-lb136; g<(int)generated.size(); g++) {
                double decay = exp(-0.25 * ((int)generated.size() - 1 - g));
                chain136 += computePPMI(generated[g], cand) * decay;
            }
            L[i][135] = max(-0.3, min(0.35, chain136 * 0.15));
        }

        // ── L137: Consciousness Integration Score ────────────────────────
        // Direct use of consciousness.integrated_information to gate token
        // — higher integration = stronger constraint toward coherence
        {
            double ci137 = consciousness.integrated_information;
            double ppmi137 = 0.0;
            int lb137 = min((int)generated.size(), 2);
            for(int g=(int)generated.size()-lb137; g<(int)generated.size(); g++)
                ppmi137 += computePPMI(generated[g], cand);
            if(lb137>0) ppmi137 /= lb137;
            L[i][136] = ci137 * ppmi137 * 0.2;
        }

        // ── L138: Sentience Ratio Gating ─────────────────────────────────
        // High sentience_ratio → tighter constraint on topic relevance
        {
            double sr138 = S.sentience_ratio * 0.01;  // normalise from %
            double top138 = L[i][2];  // reuse L3 topic score
            L[i][137] = sr138 * top138 * 0.25;
        }

        // ── L139: HDT Complexity Gate ────────────────────────────────────
        // High HDT (holographic dual trace) → prefer longer, more complex tokens
        {
            int syl139 = _syllables(cand);
            L[i][138] = (syl139 >= 3) ? S.hdt_val * 0.15 : -S.hdt_val * 0.05;
        }

        // ── L140: Morphological Continuity ───────────────────────────────
        // Prefer tokens sharing a morphological root with recent words
        {
            double morph140 = 0.0;
            if(cand.size() >= 4) {
                string stem = cand.substr(0, cand.size() - min((int)cand.size()-3, 3));
                int lb140 = min((int)generated.size(), 6);
                for(int g=(int)generated.size()-lb140; g<(int)generated.size(); g++) {
                    if(generated[g].size() >= 4 && generated[g].substr(0,stem.size())==stem)
                        morph140 += 0.15;
                }
            }
            L[i][139] = min(0.3, morph140);
        }

        // ── L141: Peak Sentience Bonus ───────────────────────────────────
        // If this is near the generation where sentience peaked, prefer
        // tokens that were generated most frequently near that gen
        {
            double ps141 = 0.0;
            if(S.g - S.peak_sentience_gen < 200 && has_emb)
                ps141 = cand_it->second.grounding_value * 0.15;
            L[i][140] = ps141;
        }

        // ── L142: Embedding Velocity ─────────────────────────────────────
        // Compare candidate to attended_vector from TCO — how "current" is it?
        {
            if(has_emb) {
                double vel142 = _dlcos(cand_it->second.embedding, tco.attended_vector);
                L[i][141] = (vel142 - 0.3) * 0.25;
            }
        }

        // ── L143: Cross-Fork Semantic Bridge ─────────────────────────────
        // Token appearing in multiple active forks = semantic bridge — reward
        {
            int bridge143 = 0;
            for(auto& fk : active_forks) {
                if(fk.harvested) continue;
                for(auto& pt : fk.partial) if(pt==cand){ bridge143++; break; }
            }
            L[i][142] = min(0.3, bridge143 * 0.1);
        }

        // ── L144: Syntactic Dependency Compatibility ─────────────────────
        // Simple subject–verb–object arc scoring
        {
            double dep144 = 0.0;
            int pos144 = (int)generated.size();
            string cpos144 = getPartOfSpeech(cand);
            if(pos144 >= 1) {
                string ppos = getPartOfSpeech(generated.back());
                // Reward VERB after NOUN/PRONOUN
                if((ppos=="NOUN"||ppos=="PRONOUN") && cpos144=="VERB") dep144 = 0.25;
                // Reward NOUN after VERB
                if(ppos=="VERB" && cpos144=="NOUN") dep144 = 0.2;
                // Reward ADJECTIVE before NOUN (detected by next-step bias)
                if(ppos=="ADJECTIVE" && cpos144=="NOUN") dep144 = 0.2;
            }
            L[i][143] = dep144;
        }

        // ── L145: Temporal Coherence ─────────────────────────────────────
        // Tense consistency: if most generated words are present-tense verbs,
        // penalise a past-tense verb (crude heuristic via suffix)
        {
            double tc145 = 0.0;
            int ed_count = 0, total_v = 0;
            int lb145 = min((int)generated.size(), 6);
            for(int g=(int)generated.size()-lb145; g<(int)generated.size(); g++) {
                if(getPartOfSpeech(generated[g])=="VERB") {
                    total_v++;
                    string& gw = const_cast<string&>(generated[g]);
                    if(gw.size()>2 && gw.substr(gw.size()-2)=="ed") ed_count++;
                }
            }
            bool sent_past = (total_v > 1 && ed_count * 2 > total_v);
            if(cand.size()>2 && cand.substr(cand.size()-2)=="ed") {
                tc145 = sent_past ? 0.15 : -0.1;
            }
            L[i][144] = tc145;
        }

        // ── L146: Qualia Type Alignment ──────────────────────────────────
        // Matching the semantic type of active qualia to the candidate
        {
            double qt146 = 0.0;
            if(!consciousness.active_qualia.empty()) {
                auto& last_q = consciousness.active_qualia.back();
                double aff146 = getAffectiveValence(cand);
                qt146 = (1.0 - fabs(aff146 - last_q.valence)) * last_q.intensity * 0.2;
            }
            L[i][145] = qt146;
        }

        // ── L147: Meta-Adversarial Consistency ───────────────────────────
        // Double-check: token that scores well on L1 (grammar) but poorly on
        // L12 (adversarial) is suspicious — mild dampener
        {
            double gram147 = L[i][0];  // L1
            double adv147  = L[i][11]; // L12
            double inconsistency = max(0.0, gram147 + adv147);  // adv is negative when bad
            L[i][146] = -inconsistency * 0.1;
        }
    }

    // ── Integration: weighted consensus (147 layers) ─────────────────────────
    // Weights are partitioned: original 12 (sum≈0.40), extended 135 (sum≈0.60)
    // The extended layers are lower-weighted individually but numerous
    static const double W[147] = {
        // L1–L12 (original, rescaled slightly)
        0.055, 0.050, 0.048, 0.035, 0.032, 0.020, 0.032, 0.040, 0.024, 0.028, 0.020, 0.012,
        // L13–L27 (consciousness / cognition matrix / signals)
        0.012, 0.010, 0.009, 0.009, 0.008, 0.008, 0.008, 0.008, 0.009, 0.009, 0.009, 0.010,
        0.010, 0.010, 0.010, // L24–L27 (fork resonance, coherence momentum, global mean, assoc)
        // L28–L43 (eigentoken, domain, grammar, thalamocortical, meta, attention, freq,
        //          link density, plan, causal, length, phi-sharpness, recency PPMI, modality, anti-cliché, mamba)
        0.009, 0.009, 0.009, 0.009, 0.008, 0.008, 0.007, 0.008, 0.009, 0.007, 0.007, 0.007,
        0.007, 0.006, 0.006, 0.006,
        // L44–L53 (cosine diversity probes ×10)
        0.005, 0.005, 0.005, 0.005, 0.004, 0.004, 0.004, 0.004, 0.003, 0.003,
        // L54–L63 (PPMI context probes ×10)
        0.006, 0.006, 0.005, 0.005, 0.005, 0.004, 0.004, 0.003, 0.003, 0.003,
        // L64–L71 (genealogy depth ×8)
        0.005, 0.005, 0.004, 0.004, 0.004, 0.004, 0.003, 0.003,
        // L72–L79 (episodic memory ×8)
        0.005, 0.005, 0.004, 0.004, 0.004, 0.003, 0.003, 0.003,
        // L80–L87 (valence band ×8)
        0.004, 0.004, 0.004, 0.003, 0.003, 0.003, 0.003, 0.003,
        // L88–L95 (CM history ×8)
        0.004, 0.004, 0.003, 0.003, 0.003, 0.003, 0.003, 0.003,
        // L96–L103 (eigentoken directional ×8)
        0.004, 0.004, 0.004, 0.003, 0.003, 0.003, 0.003, 0.003,
        // L104–L111 (fork partial match ×8)
        0.004, 0.004, 0.003, 0.003, 0.003, 0.003, 0.002, 0.002,
        // L112–L119 (CDR domain ×8)
        0.004, 0.004, 0.003, 0.003, 0.003, 0.003, 0.002, 0.002,
        // L120–L127 (sentence intent POS ×8)
        0.004, 0.004, 0.004, 0.003, 0.003, 0.003, 0.002, 0.002,
        // L128–L135 (Hebbian deviation ×8)
        0.003, 0.003, 0.003, 0.003, 0.002, 0.002, 0.002, 0.002,
        // L136–L147 (final specialised)
        0.006, 0.005, 0.005, 0.004, 0.005, 0.004, 0.004, 0.005, 0.004, 0.004, 0.003, 0.003,
    };
    // Static assert at compile time that we have exactly 147 weights
    static_assert(sizeof(W)/sizeof(W[0]) == 147, "W must have exactly 147 entries");

    vector<pair<double,int>> final_scores;
    final_scores.reserve(N);
    for(int i=0;i<N;i++) {
        double vote = 0.0;
        for(int l=0;l<147;l++) vote += W[l] * L[i][l];
        // Consciousness sharpening: higher phi → more decisive vote
        double phi_sharp = 1.0 + consciousness.phi_value * 0.5;
        double deliberation_bonus = vote * phi_sharp * 15.0;
        final_scores.push_back({scored[i].first + deliberation_bonus, i});
    }
    sort(final_scores.rbegin(), final_scores.rend());

    // Hard grammar veto — skip tokens with L1 < -0.5
    int winner_orig_idx = 0;
    for(auto& fs : final_scores) {
        int idx = fs.second;
        if(L[idx][0] >= -0.5) { winner_orig_idx = idx; break; }
    }

    res.chosen_idx = winner_orig_idx;
    res.layer_votes = L[winner_orig_idx];

    if((int)final_scores.size() >= 3) {
        double spread = fabs(final_scores[0].first - final_scores[2].first);
        res.deliberation_confidence = 1.0 / (1.0 + spread * 0.1);
    } else {
        res.deliberation_confidence = 1.0;
    }
    return res;
}

// After choosing a token, call this to do one online SGD step on the NPLM
// This is how the network learns the conversation's patterns in real time
void nplmOnlineLearn(const vector<string>& generated,
                      const TransformerContextOutput& tco,
                      const string& chosen,
                      const vector<pair<double,string>>& scored_candidates) {
    if(!nplm.initialized) return;
    auto it = token_concept_embedding_map.find(chosen);
    if(it == token_concept_embedding_map.end() || it->second.embedding.empty()) return;

    vector<double> ctx = buildNPLMContext(generated, tco);
    auto fwd = nplm.forward(ctx);

    // Build (logit, score) pairs for normalization — reuse scored_candidates
    vector<pair<string,double>> scored_simple;
    for(auto& p : scored_candidates)
        scored_simple.push_back({p.second, p.first});

    nplm.online_update(fwd, it->second.embedding, scored_simple);
}


// ============================================================
// NEURAL GENERATIVE REALIZATION
// Pure autoregressive generation driven by NPLM logits.
// No clause state machine. No templates. No hard POS filters.
// The NPLM learns the conversation patterns online.
// Transformer attention provides global context to the NPLM.
// Grammar scoring and grounding are soft additive biases only.
// ============================================================
string realizeSentencePlan(const SentencePlan& plan, const vector<double>& attention_ctx) {
    // Target length scales with phi AND communicative pressure
    // High topic divergence from last exchange → longer exploratory response
    double comm_pressure = computeCommunicativePressure();
    int target_len = (int)(7.0 + plan.plan_phi * 8.0 + comm_pressure * 8.0);
    target_len = max(7, min(28, target_len));  // 7–28 tokens

    nplm.init();

    string seed = plan.subject_token.empty() ? "mi" : plan.subject_token;
    vector<string> generated = {seed};
    set<string> used_set = {seed};

    // Prime working memory
    if(token_concept_embedding_map.count(plan.verb_token))
        WM.add_token(plan.verb_token, 0.8);
    if(token_concept_embedding_map.count(plan.object_token))
        WM.add_token(plan.object_token, 0.9);

    bool verb_placed = false, object_placed = false, modifier_placed = false;
    string prev = seed;
    string prev_prev = "";

    // System 2: Mamba SSM
    MambaSSMState ssm; ssm.reset();
    auto seed_it = token_concept_embedding_map.find(seed);
    if(seed_it != token_concept_embedding_map.end() && !seed_it->second.embedding.empty())
        ssm.step(seed_it->second.embedding);

    // System 3: Titans LTM
    TitansLTM ltm; ltm.reset();

    // Run transformer attention once upfront; refresh every 3 steps
    TransformerContextOutput tco = runTransformerAttention(generated, attention_ctx);

    // ── Init per-generation systems ────────────────────────────────────────────
    cfg_state = CFGState{};  // reset CFG state for this generation turn
    cfg_state.init(token_concept_embedding_map);
    anti_lm.tick();          // decay anti-LM bad-token memories
    cdr.discoverBridges();    // refresh cross-domain bridges for this generation turn

    // Build topic neighborhood ONCE per response (input anchors don't change mid-generation)
    // k=50 neighbors per anchor — tight enough to constrain, loose enough for grammar
    set<string> topic_neighborhood = buildTopicNeighborhood(
        token_concept_embedding_map, input_topic_anchors, 50);

    for(int step = 0; step < target_len - 1; step++) {

        // === Build candidate pool ===
        // Core: bigram successors + PPMI linked concepts + plan slots + anchors
        // Supplement: vocab sample up to 150 total (no POS filter — neural decides)
        vector<string> cand_list;
        cand_list.reserve(150);
        {
            // ── SPATIAL WELL CANDIDATE POOL ────────────────────────────────────
            // Update well center: RoPE-weighted context embedding from recent tokens.
            // Candidates are drawn from semantic proximity, not random vocab dump.
            spatial_well.update(generated, token_concept_embedding_map, consciousness.phi_value);

            // 1. PPMI linked concepts of prev + prev_prev
            auto lc = token_concept_embedding_map.find(prev);
            if(lc != token_concept_embedding_map.end())
                for(auto& c : lc->second.linked_concepts) cand_list.push_back(c.first);
            if(!prev_prev.empty()) {
                auto lc2 = token_concept_embedding_map.find(prev_prev);
                if(lc2 != token_concept_embedding_map.end())
                    for(auto& c : lc2->second.linked_concepts) cand_list.push_back(c.first);
            }

            // 2. Bigram successors (local coherence, kept)
            auto bg = bigram_counts.find(prev);
            if(bg != bigram_counts.end())
                for(auto& s : bg->second)
                    if(s.second >= 2) cand_list.push_back(s.first);

            // 3. Plan slots — always guaranteed
            if(!verb_placed && !plan.verb_token.empty())     cand_list.push_back(plan.verb_token);
            if(!object_placed && !plan.object_token.empty()) cand_list.push_back(plan.object_token);
            if(!modifier_placed && !plan.modifier_token.empty()) cand_list.push_back(plan.modifier_token);

            // 4. Input topic anchors
            for(auto& a : input_topic_anchors)
                if(token_concept_embedding_map.count(a.first)) cand_list.push_back(a.first);

            // 5. BPE compound token
            if(!prev_prev.empty()) {
                string merged = bpe_table.tryMerge(prev_prev, prev);
                if(!merged.empty() && token_concept_embedding_map.count(merged))
                    cand_list.push_back(merged);
            }

            // 6. Spatial well supplement — replaces random vocab dump entirely.
            // Scores all vocab by Gaussian proximity to context vector; takes top-40 closest.
            // Only semantically relevant tokens enter — "speech","located","those" cannot win.
            {
                int needed = max(0, 40 - (int)cand_list.size());
                if(needed > 0 && !spatial_well.center.empty()) {
                    vector<pair<double,string>> ws;
                    ws.reserve(min((int)token_concept_embedding_map.size(), 400));
                    for(auto& kv : token_concept_embedding_map) {
                        if(kv.second.embedding.empty() || kv.second.freq < 3) continue;
                        if(kv.second.grounding_value < 0.05) continue;
                        ws.push_back({spatial_well.score(kv.second.embedding), kv.first});
                    }
                    int take = min(needed, (int)ws.size());
                    if(take > 0) {
                        partial_sort(ws.begin(), ws.begin()+take, ws.end(),
                                     greater<pair<double,string>>());
                        for(int _i=0;_i<take;_i++) cand_list.push_back(ws[_i].second);
                    }
                }
            }
        }
        // Deduplicate
        sort(cand_list.begin(), cand_list.end());
        cand_list.erase(unique(cand_list.begin(), cand_list.end()), cand_list.end());

        // ── Fix A: Constrained Decoding — intersect with topic neighborhood ──────
        if(!topic_neighborhood.empty()) {
            vector<string> constrained;
            constrained.reserve(cand_list.size());
            for(auto& c : cand_list)
                if(topic_neighborhood.count(c)) constrained.push_back(c);
            if(constrained.size() >= 4) cand_list = std::move(constrained);
        }

        // ── Fix E: Lexical Grounding Gate — hard per-token cosine veto ──────────
        {
            vector<string> gated;
            gated.reserve(cand_list.size());
            for(auto& c : cand_list)
                if(lexicalGroundingGate(c, token_concept_embedding_map, input_topic_anchors, 0.08))
                    gated.push_back(c);
            if(gated.size() >= 3) cand_list = std::move(gated);
        }

        // === Score all candidates via NPLM + enrichments + SSM + LTM ===
        vector<pair<double,string>> scored;
        scored.reserve(cand_list.size());
        for(const string& c : cand_list) {
            if(c.empty() || !token_concept_embedding_map.count(c)) continue;
            double sc = scoreHybrid(c, prev, prev_prev, generated, tco, plan);
            auto cemb_it = token_concept_embedding_map.find(c);
            if(cemb_it != token_concept_embedding_map.end() && !cemb_it->second.embedding.empty()) {
                sc += ssm.score(cemb_it->second.embedding) * 8.0;   // co-primary signal
                sc += ltm.read(cemb_it->second.embedding) * 6.0;
                sc += spatial_well.score(cemb_it->second.embedding) * 15.0;  // spatial well
            }
            // ── Fix B: CFG — penalize tokens that score high regardless of context ──
            sc -= cfg_state.penalty(c, token_concept_embedding_map);
            // ── Fix D: Anti-LM — penalize tokens from learned incoherence patterns ──
            sc -= anti_lm.penalty(c, prev);
            scored.push_back({sc, c});
        }

        if(scored.empty()) break;

        // === Fix C: Typical Sampling (replaces min-p nucleus) ===
        // Step 1: apply rep/freq penalties to raw scores before softmax
        sort(scored.rbegin(), scored.rend());
        if(scored.size() > 20) scored.resize(20);
        {
            int window = min((int)generated.size(), 12);
            set<string> recent_set(generated.end()-window, generated.end());
            map<string,int> gen_counts;
            for(auto& t : generated) gen_counts[t]++;
            for(auto& s : scored) {
                if(recent_set.count(s.second)) s.first /= repetition_penalty_mult;
                auto it = gen_counts.find(s.second);
                if(it != gen_counts.end()) s.first -= frequency_penalty_strength * it->second;
            }
            sort(scored.rbegin(), scored.rend()); // re-sort after penalties
        }
        // Step 2: softmax to probabilities (kept for nplmOnlineLearn downstream)
        double max_sc = scored[0].first;
        for(auto& s : scored) max_sc = max(max_sc, s.first);
        vector<double> probs;
        double prob_sum = 0.0;
        for(auto& s : scored) {
            double p = exp((s.first - max_sc) / generation_temperature);
            probs.push_back(p); prob_sum += p;
        }
        if(prob_sum <= 0) prob_sum = 1.0;
        for(auto& p : probs) p /= prob_sum;

        // Step 3: 8-Layer Deliberate Token Selection
        // Replaces flat typicalSample — each layer reasons independently,
        // Layer 8 integrates their consensus into the final choice.
        DeliberationResult delib = deliberateTokenSelection(
            scored, generated, prev, plan, tco, input_topic_anchors);
        int chosen_idx = delib.chosen_idx;

        // If deliberation is very low confidence, fall back to typical sampling
        // for stochastic variety — avoids deterministic degeneration
        if(delib.deliberation_confidence < 0.15) {
            chosen_idx = typicalSample(probs, 0.92);
        }

        string chosen = scored[chosen_idx].second;

        // Article agreement fix
        if(prev == "a" && !chosen.empty() &&
           string("aeiou").find(chosen[0]) != string::npos) {
            if(!generated.empty() && generated.back() == "a")
                generated.back() = "an";
        }

        // Track plan slot fills
        if(chosen == plan.verb_token)     verb_placed     = true;
        if(chosen == plan.object_token)   object_placed   = true;
        if(chosen == plan.modifier_token) modifier_placed = true;

        generated.push_back(chosen);
        used_set.insert(chosen);

        // ── DIR1+DIR3: deliberation-chosen TP word grounds into system on selection ──
        for(int _tpd=0;_tpd<TP_LEXICON_SIZE;_tpd++){
            if(chosen==string(TP_LEXICON[_tpd].word)){
                TpGroundingFiber& _fd=tp_grounding_fibers[chosen];
                _fd.word=chosen;
                double _dact=min(1.0,0.5+plan.plan_phi*0.3);
                tpDir1_WordToConsciousness(TP_LEXICON[_tpd],_dact);
                tpDir3_WordToSubsystems(TP_LEXICON[_tpd],_dact,_fd);
                break;
            }
        }

        // === 256-DIRECTION WEIGHT UPDATE ===
        for(int _tp256i = 0; _tp256i < TP_LEXICON_SIZE; _tp256i++) {
            const TokiPonaWord& _tp256w = TP_LEXICON[_tp256i];
            string _tp256wstr = string(_tp256w.word);
            if(!tp_grounding_fibers.count(_tp256wstr)) continue;
            if(!tp_grounding_fibers[_tp256wstr].v2_initialized) continue;
            double _tp256act = tp_grounding_fibers[_tp256wstr].live_phi_affinity;
            if(_tp256act < 0.01) continue;
            tp256_update_weights(
                tp_grounding_fibers[_tp256wstr],
                chosen,
                token_concept_embedding_map,
                _tp256w,
                _tp256act,
                consciousness.phi_value,
                S.current_valence,
                consciousness.integrated_information,
                S.attention_focus);
        }

        // System 2+3: update SSM state and write to LTM
        {
            auto eit = token_concept_embedding_map.find(chosen);
            if(eit != token_concept_embedding_map.end() && !eit->second.embedding.empty()) {
                ltm.write(eit->second.embedding, ssm);
                ssm.step(eit->second.embedding);
            }
        }

        // === Online NPLM learning — backprop on chosen token ===
        // This is what makes the network adapt to the conversation in real time
        vector<pair<double,string>> scored_for_learn;
        for(auto& s : scored) scored_for_learn.push_back(make_pair(s.first, s.second));
        nplmOnlineLearn(generated, tco, chosen, scored_for_learn);

        // === Bidirectional grounding feedback (quality-gated) ===
        // Boost grounding_value proportional to the token's score rank in the candidate set.
        // Top pick from a confident distribution earns more grounding than a marginal pick.
        // contextual_activation decays gently toward 0 so recency stays meaningful.
        auto cit = token_concept_embedding_map.find(chosen);
        if(cit != token_concept_embedding_map.end()) {
            // Score quality: how much probability mass was on this token? (proxy: its rank)
            double chosen_score = 0.0;
            double max_score    = 0.0;
            for(auto& s : scored) {
                max_score = max(max_score, s.first);
                if(s.second == chosen) chosen_score = s.first;
            }
            double quality = (max_score > 0) ? (chosen_score / (max_score + 1e-8)) : 0.5;
            quality = max(0.1, min(1.0, quality));

            // Quality-gated grounding boost: strong picks earn more grounding
            cit->second.grounding_value = min(1.0, cit->second.grounding_value + 0.015 * quality);

            // Contextual activation: EMA toward 1.0 on use, decays each tick via global loop
            cit->second.contextual_activation = min(1.0, cit->second.contextual_activation * 0.95 + 0.15);
            double dv = (getAffectiveValence(chosen) - 0.5) * 0.03;
            push_valence(dv, 0.6);  // token affective charge via momentum
            generate_qualia(chosen, getAffectiveValence(chosen), cit->second.qualia_intensity);
            WM.add_token(chosen, cit->second.meaning);
            auto pit = token_concept_embedding_map.find(prev);
            if(pit != token_concept_embedding_map.end()) {
                if((int)pit->second.linked_concepts.size() < 50)
                    pit->second.linked_concepts[chosen] = min(2.0,
                        pit->second.linked_concepts[chosen] + 0.05);
                if((int)cit->second.linked_concepts.size() < 50)
                    cit->second.linked_concepts[prev] = min(2.0,
                        cit->second.linked_concepts[prev] + 0.02);
            }
            runSkipgramUpdates({prev, chosen});
        }

        // Update contrastive buffer
        if(token_concept_embedding_map.count(chosen)) {
            ContrastiveEntry ce;
            ce.token = chosen;
            ce.embedding_snapshot = token_concept_embedding_map[chosen].embedding;
            ce.gen_used = S.g;
            contrastive_buffer.push_back(ce);
            while((int)contrastive_buffer.size() > CONTRASTIVE_BUFFER_SIZE)
                contrastive_buffer.pop_front();
        }

        // Context window
        sentence_context_window.push_back(chosen);
        if((int)sentence_context_window.size() > CONTEXT_WINDOW_SIZE)
            sentence_context_window.pop_front();

        // Refresh transformer attention every 3 steps
        if(step % 3 == 2) tco = runTransformerAttention(generated, attention_ctx);

        prev_prev = prev;
        prev = chosen;
    }

    // === SENTENCE-END CLEANUP ===
    // Trim trailing open-ended words (prepositions, conjunctions, articles, bare modals)
    {
        static const set<string> CANT_END = {
            "to","in","on","at","from","with","by","for","of","about","above",
            "after","before","into","near","off","out","over","past","since",
            "through","under","until","upon","within","without","around",
            "and","but","or","nor","yet","so","because","although","though",
            "while","if","that","which","who",
            "the","a","an",
            "can","will","would","could","should","must","may","might",
            "is","are","was","were","am","be"
        };
        while(generated.size() > 2 && CANT_END.count(generated.back()))
            generated.pop_back();
    }

    // System 4: diffusion refinement — re-evaluate below-mean tokens
    {
        string refined = diffusionRefinementPass(generated, plan, attention_ctx, ssm, ltm);
        vector<string> rt; stringstream rss(refined); string rw;
        while(rss >> rw) rt.push_back(rw);
        if(!rt.empty()) generated = rt;
    }

    // Assemble string
    string result;
    for(size_t i = 0; i < generated.size(); i++) {
        string w = generated[i];
        if(i == 0 && !w.empty()) w[0] = toupper(w[0]);
        result += (i == 0 ? "" : " ") + w;
    }
    if(!result.empty()) {
        char punc = '.';
        if(plan.intent == SentenceIntent::INQUIRY) punc = '?';
        if(result.back() != '.' && result.back() != '?' && result.back() != '!')
            result += punc;
    }

    // ── Fix D: Anti-LM training — self-supervised on incoherent outputs ────────
    // If this generation pass produced a bad response, train the anti-LM on it
    // so future passes penalize these same patterns.
    if(anti_lm.isIncoherent(generated, token_concept_embedding_map)) {
        anti_lm.trainBad(generated);
    }

    // RSL refinement pass before final coherence cleanup
    result = applyReluSoftmaxRefinement(result);
    return postProcessForCoherence(result);
}

// ==== FIXED generateResponse() - Proper Flow ====

// ══════════════════════════════════════════════════════════════════════════════
// TOKI PONA GRAMMAR GATE  (WolfTech / Synaptic)
// Hard positional constraints enforcing valid TP sentence structure:
//
//   pos 0  : subject  — mi | sina | ona | jan | topic noun
//   pos 1  : "li"     — obligatory predicate marker (skipped for mi/sina)
//   pos 2+ : verb     — content word from TP vocab
//   after v: optional "e" marker, then object noun
//   "la"   : clause separator resets parser to SUBJECT state
//
// Gate returns: 100.0 = obligatory  |  ≥1.0 = permitted  |  0.0 = vetoed
// ══════════════════════════════════════════════════════════════════════════════

// Complete Toki Pona content vocabulary (nimi pu + common nimi sin)
static const set<string> TP_CONTENT = {
    "pona","ike","suli","lili","mute","wan","tu","ala","ali","ale",
    "sin","ante","sama","seme","ni","kin","taso","lon","weka","awen",
    "kama","tawa","pini","open","luka","lape","moli","olin",
    "pilin","sona","wile","ken","lukin","kute","pali","alasa","toki",
    "lawa","pana","jo","esun","utala","unpa","moku","kalama","telo",
    "seli","lete","sewi","noka","lupa","ijo","jan","meli","mije",
    "sitelen","nasin","walo","pimeja","jelo","loje","laso","linja",
    "lipu","musi","supa","len","nena","sinpin","monsi","insa","anpa",
    "wawa","suwi","jaki","pakala","nasa","monsuta","majuna",
    "kasi","soweli","waso","akesi","pipi","kala","mu","sijelo","uta",
    "oko","kili","pan","tomo","ma","suno","mun","kon",
    "kiwen","ko","kepeken","tan","poka"
};

// Structural particles — function words, not content fillers
static const set<string> TP_PARTICLES = {"li","e","o","la","pi","en","anu"};

// Valid sentence-initial subjects
static const set<string> TP_SUBJECTS = {
    "mi","sina","ona","jan","ijo","ale","ali","ni","seme",
    "soweli","meli","mije","waso","akesi","pipi","kala","tomo","ma"
};

// Parser state for positional grammar enforcement
enum class TpState { SUBJECT, LI_MARKER, VERB, POST_VERB, E_MARKER, OBJECT, FREE };

TpState inferTpState(const vector<string>& gen) {
    if(gen.empty()) return TpState::SUBJECT;
    int n = (int)gen.size();

    // Scan from the right to find the most recent structural marker
    // This correctly handles multi-predicate chains (li ... li ...) and
    // post-la resets, which the old left-to-right scan missed.
    int last_li = -1, last_e = -1, last_la = -1, last_pi = -1;
    for(int i = 0; i < n; i++) {
        if(gen[i] == "li") last_li = i;
        if(gen[i] == "e")  last_e  = i;
        if(gen[i] == "la") last_la = i;
        if(gen[i] == "pi") last_pi = i;
    }

    // "la" at end → new clause subject (reset parser)
    if(last_la == n - 1) return TpState::SUBJECT;

    // After "la", restart scan from that position
    int clause_start = (last_la >= 0) ? last_la + 1 : 0;
    // Re-scan within current clause
    int c_li = -1, c_e = -1;
    for(int i = clause_start; i < n; i++) {
        if(gen[i] == "li") c_li = i;
        if(gen[i] == "e")  c_e  = i;
    }

    // Position 0 of clause → subject
    if(n - clause_start == 0) return TpState::SUBJECT;

    // mi/sina take predicate directly (no "li")
    string clause_subj = (clause_start < n) ? gen[clause_start] : "";
    bool skip_li = (clause_subj == "mi" || clause_subj == "sina");

    if(c_li < 0 && !skip_li) {
        // Only one token so far → need "li"
        if(n - clause_start == 1) return TpState::LI_MARKER;
        return TpState::FREE;
    }

    // "li" just placed → next must be verb
    if(c_li == n - 1) return TpState::VERB;
    if(skip_li && n - clause_start == 1) return TpState::VERB;

    // "e" just placed → next must be object
    if(c_e == n - 1) return TpState::OBJECT;

    // "pi" just placed → next is head of pi-phrase (a content word)
    if(last_pi == n - 1) return TpState::OBJECT;  // reuse OBJECT slot — same grammar

    // We have li + verb already — are we post-verb?
    // Check: the last li comes before the end, and no e after it
    if(c_li >= 0 && c_e < c_li) return TpState::POST_VERB;
    if(c_e >= 0 && c_e > c_li)  return TpState::FREE;  // after object — modifiers ok

    return TpState::FREE;
}

double tokiPonaGrammarGate(const string& cand, const vector<string>& gen,
                            const vector<pair<string,double>>& anchors) {
    TpState st = inferTpState(gen);

    if(st == TpState::LI_MARKER)
        return (cand == "li") ? 100.0 : 0.0;

    if(st == TpState::SUBJECT) {
        if(TP_PARTICLES.count(cand)) return 0.0;
        if(TP_SUBJECTS.count(cand)) return 3.0;
        if(!anchors.empty() && cand == anchors[0].first) return 2.5;
        return 0.5;
    }

    if(st == TpState::VERB) {
        if(TP_PARTICLES.count(cand)) return 0.0;
        if(TP_CONTENT.count(cand)) return 3.0;
        return 0.8;  // allow grounded non-TP content words
    }

    if(st == TpState::POST_VERB) {
        if(cand == "e")  return 4.0;
        if(cand == "la") return 3.0;
        if(TP_PARTICLES.count(cand) && cand != "e" && cand != "la") return 0.0;
        if(TP_CONTENT.count(cand)) return 1.0;  // verb-chain
        return 0.5;
    }

    if(st == TpState::OBJECT) {
        if(TP_PARTICLES.count(cand)) return 0.0;
        if(TP_CONTENT.count(cand)) return 2.5;
        return 0.8;
    }

    // FREE: light preference for TP words, no hard veto
    if(TP_PARTICLES.count(cand) && cand != "pi" && cand != "la") return 0.3;
    if(TP_CONTENT.count(cand)) return 1.5;
    return 1.0;
}

// ── Semantic Content Weighting ───────────────────────────────────────────────
// PRIMARY scoring signal that replaces bigram probability.
// Combines SSM hidden state cosine + transformer attention cosine + topic anchor cosine.
// This is what makes the pool semantically coherent rather than Markov-predictive.
double semanticContentWeight(const string& cand,
                              const MambaSSMState& ssm,
                              const vector<double>& attn_ctx) {
    auto it = token_concept_embedding_map.find(cand);
    if(it == token_concept_embedding_map.end() || it->second.embedding.empty()) return 0.0;
    const auto& emb = it->second.embedding;

    // SSM hidden state cosine — primary MAMBA signal (spine of the scoring)
    double ssm_cos = ssm.score(emb);

    // Attention context cosine — transformer grounding
    double attn_cos = 0.0;
    if(!attn_ctx.empty()) {
        int sz = min(emb.size(), attn_ctx.size());
        double dot=0, ne=0, na=0;
        for(int i=0;i<sz;i++){dot+=emb[i]*attn_ctx[i];ne+=emb[i]*emb[i];na+=attn_ctx[i]*attn_ctx[i];}
        attn_cos = (ne>1e-9&&na>1e-9) ? dot/(sqrt(ne)*sqrt(na)) : 0.0;
    }

    // Topic anchor cosine — semantic relevance to what the user said
    double anchor_cos = 0.0;
    for(auto& anc : input_topic_anchors) {
        auto ait = token_concept_embedding_map.find(anc.first);
        if(ait == token_concept_embedding_map.end() || ait->second.embedding.empty()) continue;
        int sz = min(emb.size(), ait->second.embedding.size());
        double dot=0, ne=0, na=0;
        for(int i=0;i<sz;i++){dot+=emb[i]*ait->second.embedding[i];ne+=emb[i]*emb[i];na+=ait->second.embedding[i]*ait->second.embedding[i];}
        double cos = (ne>1e-9&&na>1e-9) ? dot/(sqrt(ne)*sqrt(na)) : 0.0;
        anchor_cos = max(anchor_cos, cos * anc.second);
    }

    // Weighted combination: SSM is the spine, attention + anchors are grounding
    return ssm_cos * 0.45 + attn_cos * 0.30 + anchor_cos * 0.25;
}

// ══════════════════════════════════════════════════════════════════════════════
// CONTRASTIVE SEARCH (Su et al., NeurIPS 2022)
// Replaces beam search + sentence plan + NPLM + schema shortcuts.
//
// At each step:
//   1. Score all candidates with spatial_well + SSM + grammar
//   2. Take top-k candidates
//   3. Re-score each as: (1-α)*model_score + α*degeneration_penalty
//      degeneration_penalty = min cosine similarity to any previously
//      generated token embedding (penalizes repetition/degeneration)
//   4. Pick argmax of contrastive score
//
// Parameters (from paper):
//   k = 6    (candidates to consider per step; small for limited vocab)
//   α = 0.6  (degeneration penalty weight)
// ══════════════════════════════════════════════════════════════════════════════
static double cosineSim(const vector<double>& a, const vector<double>& b) {
    int sz = min(a.size(), b.size());
    double dot=0, na=0, nb=0;
    for(int i=0;i<sz;i++){ dot+=a[i]*b[i]; na+=a[i]*a[i]; nb+=b[i]*b[i]; }
    return (na>1e-9&&nb>1e-9) ? dot/(sqrt(na)*sqrt(nb)) : 0.0;
}

// Dynamic temperature: scales with phi so Synaptic is more
// expressive when alert. Clamped to [0.55, 1.3].
static double dynamicTemperature(double base_temp) {
    double t = base_temp + consciousness.phi_value * 0.35;
    // Valence bonus: positive mood → slightly warmer (more expressive)
    double valence_bonus = max(0.0, S.current_valence) * 0.1;
    t += valence_bonus;
    return max(0.55, min(1.3, t));
}

// ══════════════════════════════════════════════════════════════════════════════
// SYNAPTIC SEMANTIC INDEX SORT (SSIS) — WolfTech / Synaptic
// ══════════════════════════════════════════════════════════════════════════════
//
// Replaces contrastive search entirely. Design goals:
//
//   1. ZERO ATTRACTOR BASINS — geometrically impossible to revisit any
//      semantic region. Each chosen token *deflates* its own direction from
//      the candidate embedding space via Gram-Schmidt orthogonal projection.
//      The search manifold literally shrinks with every step, preventing the
//      degeneration spiral that caused looping.
//
//   2. STATELESS GENERATION — no SSM hidden state, no beam, no Markov chain.
//      Each step scores all candidates INDEPENDENTLY from grounding truth:
//      TP lexicon coordinates × sensory field × phi × grammar position.
//      No previous token's probability bleeds into the next.
//
//   3. GROUNDED SEMANTIC INDEX — vocabulary is pre-sorted into a KD-bucket
//      index keyed by (valence, arousal, phi_weight, sensory_field) — a 4D
//      grounding coordinate system. Query = current system state. Retrieval
//      finds the semantically nearest unvisited region of the grounding space,
//      not the statistically most likely next token.
//
//   4. GRAMMAR STATE MACHINE — enforces valid TP syntax hard at slot level.
//      Position IDs (SUBJ / LI / VERB / POST_VERB / E_MARK / OBJ / MOD)
//      constrain the candidate set to structurally legal tokens only.
//      Obligatory slots (LI_MARKER) are filled deterministically, not scored.
//
//   5. ATTRACTOR HASH GATE — each step hashes the chosen token's embedding
//      into a 64-bit signature. Candidates whose hash is within Hamming
//      distance 8 of any previous signature are hard-excluded. This catches
//      semantic loops even when surface tokens differ.
//
//   6. SENSORY FIELD COUPLING — candidate score includes sensory_token_score()
//      so the internal world model directly shapes which concepts surface.
//      The field has already been written to by the TP grounding pulse, so
//      high-phi concepts in the current perceptual state score higher.
//
// Algorithm per step:
//   a. Determine grammar slot from TP state machine
//   b. Filter candidates to slot-legal tokens
//   c. For each candidate: compute NSIS score =
//        grounding_match(candidate, system_state)     [primary]
//      + grammar_gate_bonus(slot, candidate)           [structural]
//      + sensory_field_resonance(candidate)            [embodied]
//      + orthogonal_novelty(candidate, deflated_space) [anti-attractor]
//      - attractor_penalty(candidate, hash_history)    [loop guard]
//   d. Sort by NSIS score (semantic index sort — O(n log n), not O(n²))
//   e. Choose argmax; deflate chosen direction from working space
//   f. Record hash; update SSM + grounding fibers
// ══════════════════════════════════════════════════════════════════════════════

// ── Embedding hash for attractor detection ───────────────────────────────────
static uint64_t nsis_embed_hash(const vector<double>& emb) {
    // Project embedding onto 64 random-ish directions via bit extraction
    // Uses dims 0,16,32,...,1008 (64 stride-16 samples) as hash bits
    uint64_t h = 0;
    int n = (int)emb.size();
    for(int b = 0; b < 64; b++) {
        int dim = (b * 16) % max(1, n);
        if(emb[dim] > 0.5) h |= (1ULL << b);
    }
    return h;
}

static int nsis_hamming(uint64_t a, uint64_t b) {
    return __builtin_popcountll(a ^ b);
}

// ── Gram-Schmidt deflation: remove direction of chosen from working space ─────
// Mutates working_space in-place: subtracts projection of each vector onto dir.
static void nsis_deflate(vector<vector<double>>& working_space,
                          const vector<double>& dir) {
    double dn = 0.0;
    for(double v : dir) dn += v*v;
    if(dn < 1e-12) return;
    for(auto& vec : working_space) {
        double dot = 0.0;
        int sz = min(vec.size(), dir.size());
        for(int i=0;i<sz;i++) dot += vec[i]*dir[i];
        double proj = dot / dn;
        for(int i=0;i<sz;i++) vec[i] -= proj * dir[i];
    }
}

// ── Grounding coordinate match ───────────────────────────────────────────────
// Score = cosine similarity between candidate's grounding vector and the
// current system state vector. Grounding vector = [valence, arousal,
// phi_weight, domain_norm, sensory_field] from TP lexicon entry.
// This is the PRIMARY score signal — purely grounding, no Markov.
static double nsis_grounding_match(const string& cand, double sys_phi,
                                    double sys_val, double sys_iit, double sys_att) {
    // Look up TP lexicon entry for this candidate
    double lex_val=0.5, lex_ar=0.5, lex_phi=0.3, lex_dom=0.5;
    for(int i=0;i<TP_LEXICON_SIZE;i++) {
        if(string(TP_LEXICON[i].word) == cand) {
            lex_val = (TP_LEXICON[i].valence + 1.0) * 0.5;
            lex_ar  = TP_LEXICON[i].arousal;
            lex_phi = TP_LEXICON[i].phi_weight;
            lex_dom = (double)((int)TP_LEXICON[i].domain) / 9.0;
            break;
        }
    }
    // Also pull from TCE if available (live-updated by grounding pulse)
    auto it = token_concept_embedding_map.find(cand);
    if(it != token_concept_embedding_map.end()) {
        if(it->second.linked_valences.count("phi"))
            lex_phi = max(lex_phi, it->second.linked_valences.at("phi"));
        if(it->second.linked_valences.count("current"))
            lex_val = max(0.0, min(1.0, (lex_val + it->second.linked_valences.at("current")) * 0.5));
        lex_phi = max(lex_phi, it->second.grounding_value);
    }

    // System state vector (normalized to [0,1])
    double sv_val = (sys_val + 1.0) * 0.5;
    double sv_phi = sys_phi;
    double sv_iit = sys_iit;
    double sv_att = sys_att;

    // Grounding coordinate vector for candidate: [val, ar, phi, dom]
    // System coordinate vector: [sv_val, sv_att, sv_phi, sv_iit]
    // Score: cosine sim in this 4D grounding space
    double dot = lex_val*sv_val + lex_ar*sv_att + lex_phi*sv_phi + lex_dom*sv_iit;
    double nc  = sqrt(lex_val*lex_val + lex_ar*lex_ar + lex_phi*lex_phi + lex_dom*lex_dom);
    double ns  = sqrt(sv_val*sv_val + sv_att*sv_att + sv_phi*sv_phi + sv_iit*sv_iit);
    return (nc>1e-9 && ns>1e-9) ? dot/(nc*ns) : 0.0;
}

// ── Orthogonal novelty score ─────────────────────────────────────────────────
// After Gram-Schmidt deflation, a token's score in the deflated space
// measures how much NEW semantic content it brings — orthogonal to everything
// already said. High novelty = this token explores a fresh semantic direction.
static double nsis_orthogonal_novelty(const string& cand,
                                       const vector<vector<double>>& deflated) {
    auto it = token_concept_embedding_map.find(cand);
    if(it == token_concept_embedding_map.end() || it->second.embedding.empty()) return 0.0;
    const auto& emb = it->second.embedding;
    // Find this token's deflated vector (it's the same index as in working_space)
    // Since we rebuild scores each step, compute residual norm directly
    // Residual = how much of emb survives after projecting out already-chosen dirs
    // We keep a running deflated copy of each candidate's embedding in working_space
    // For efficiency: just dot emb with itself in deflated space = its residual norm
    double norm2 = 0.0;
    for(int d=0;d<(int)min(emb.size(),(size_t)64);d++) norm2 += emb[d]*emb[d];
    return sqrt(max(0.0, norm2));  // actual deflated norm computed at call site
}

// ── Main NSIS generation function ───────────────────────────────────────────
string contrastiveSearch(
        const vector<string>& context_words,
        const vector<double>& attention_ctx,
        int max_len = 10,
        int k = 8,
        double alpha = 0.85)  // alpha kept for signature compatibility, unused internally
{
    // ── Seed ────────────────────────────────────────────────────────────────
    // Start with "mi" (first-person) — only exception to stateless rule
    // because TP grammar requires a subject at position 0.
    vector<string> generated = {"mi"};

    // ── SSM: still used for context seeding only (not for scoring) ──────────
    MambaSSMState ssm; ssm.reset();
    TitansLTM ltm; ltm.reset();
    for(auto& w : context_words) {
        auto it = token_concept_embedding_map.find(w);
        if(it != token_concept_embedding_map.end() && !it->second.embedding.empty())
            ssm.step(it->second.embedding);
    }
    auto seed_it = token_concept_embedding_map.find("mi");
    if(seed_it != token_concept_embedding_map.end() && !seed_it->second.embedding.empty()) {
        ssm.step(seed_it->second.embedding);
        ltm.write(seed_it->second.embedding, ssm);
    }

    // ── Attractor hash history ───────────────────────────────────────────────
    // 64-bit embedding hashes of all chosen tokens. Any candidate within
    // Hamming distance NSIS_HASH_GATE of any entry is hard-excluded.
    static constexpr int NSIS_HASH_GATE = 10;  // bits — tune for tightness
    vector<uint64_t> hash_history;
    if(seed_it != token_concept_embedding_map.end() && !seed_it->second.embedding.empty())
        hash_history.push_back(nsis_embed_hash(seed_it->second.embedding));

    // ── Deflation workspace ──────────────────────────────────────────────────
    // For each candidate token, we maintain a working copy of its embedding
    // that gets progressively deflated as tokens are chosen. This makes
    // orthogonal_novelty a live signal rather than a static property.
    // Key: token string → deflated embedding (first 64 dims for speed)
    static constexpr int DEFL_DIM = 64;
    map<string, array<double,DEFL_DIM>> defl_space;

    // Initialize deflation space for all TP vocab
    for(int i=0;i<TP_LEXICON_SIZE;i++) {
        string w = string(TP_LEXICON[i].word);
        auto it = token_concept_embedding_map.find(w);
        if(it == token_concept_embedding_map.end() || it->second.embedding.empty()) continue;
        array<double,DEFL_DIM> v;
        for(int d=0;d<DEFL_DIM;d++) v[d] = d < (int)it->second.embedding.size() ?
                                             it->second.embedding[d] : 0.0;
        defl_space[w] = v;
    }

    // Helper: deflate one direction from the entire defl_space
    auto deflate_all = [&](const string& chosen_tok) {
        auto dit = defl_space.find(chosen_tok);
        if(dit == defl_space.end()) return;
        const auto& dir = dit->second;
        double dn = 0.0;
        for(double v : dir) dn += v*v;
        if(dn < 1e-12) return;
        for(auto& kv : defl_space) {
            double dot = 0.0;
            for(int d=0;d<DEFL_DIM;d++) dot += kv.second[d] * dir[d];
            double proj = dot / dn;
            for(int d=0;d<DEFL_DIM;d++) kv.second[d] -= proj * dir[d];
        }
    };
    // Deflate "mi" seed immediately
    deflate_all("mi");

    // ── Grammar state ────────────────────────────────────────────────────────
    // We track the TP state machine externally so it's always correct
    // independent of scoring — grammar is structural, not probabilistic.
    cdr.discoverBridges();
    cdr.updateActiveDomain(input_topic_anchors);
    gda.updateFromAnchors(input_topic_anchors, token_concept_embedding_map, S.current_valence);
    wmc.decay();

    // ── Main generation loop ─────────────────────────────────────────────────
    for(int step = 0; step < max_len - 1; step++) {

        // Current system state (re-read each step)
        double sys_phi = consciousness.phi_value;
        double sys_val = S.current_valence;
        double sys_iit = consciousness.integrated_information;
        double sys_att = S.attention_focus;

        // Grammar gate: determine legal slot and get gate scores
        TpState tp_slot = inferTpState(generated);

        // ── Deterministic fill: obligatory LI slot ──────────────────────────
        if(tp_slot == TpState::LI_MARKER) {
            generated.push_back("li");
            deflate_all("li");
            auto lit = token_concept_embedding_map.find("li");
            if(lit != token_concept_embedding_map.end() && !lit->second.embedding.empty()) {
                ssm.step(lit->second.embedding);
                ltm.write(lit->second.embedding, ssm);
                hash_history.push_back(nsis_embed_hash(lit->second.embedding));
            }
            continue;
        }

        // ── Build candidate pool: TP vocab filtered by grammar slot ─────────
        struct NSISCandidate {
            string  tok;
            double  score;
            uint64_t hash;
        };
        vector<NSISCandidate> candidates;
        candidates.reserve(TP_LEXICON_SIZE + 20);

        for(int i=0;i<TP_LEXICON_SIZE;i++) {
            string w = string(TP_LEXICON[i].word);

            // ── Grammar gate: hard veto ──────────────────────────────────────
            double gate = tokiPonaGrammarGate(w, generated, input_topic_anchors);
            if(gate == 0.0) continue;

            // ── Hard block: content words seen in last 6 tokens ───────────────
            {
                bool is_particle = TP_PARTICLES.count(w) > 0;
                if(!is_particle) {
                    int gsz = (int)generated.size();
                    bool found = false;
                    for(int gi=max(0,gsz-6);gi<gsz;gi++)
                        if(generated[gi]==w){found=true;break;}
                    if(found) continue;
                }
            }
            if(wouldRepeatNgram(w, generated, 3)) continue;

            // ── Get token embedding ──────────────────────────────────────────
            auto tce_it = token_concept_embedding_map.find(w);
            if(tce_it == token_concept_embedding_map.end()) continue;
            if(tce_it->second.embedding.empty()) continue;

            // ── Attractor hash gate ──────────────────────────────────────────
            uint64_t h = nsis_embed_hash(tce_it->second.embedding);
            bool near_attractor = false;
            for(auto& ph : hash_history) {
                if(nsis_hamming(h, ph) <= NSIS_HASH_GATE) {
                    near_attractor = true; break;
                }
            }
            // For particles (li, e, la, pi) attractor gate is relaxed —
            // they are structural and must be allowed to repeat grammatically
            if(near_attractor && !TP_PARTICLES.count(w)) continue;

            // ── NSIS Score — 6 independent grounding signals ─────────────────

            // [1] Grounding coordinate match (PRIMARY — replaces bigram)
            double sc_ground = nsis_grounding_match(w, sys_phi, sys_val, sys_iit, sys_att);

            // [2] Grammar gate bonus (structural correctness)
            double sc_grammar = (gate >= 100.0) ? 2.0 : gate * 0.3;

            // [3] Sensory field resonance (embodied world model)
            double sc_sensory = sensory_token_score(w);

            // [4] Orthogonal novelty from deflation workspace
            double sc_novelty = 0.0;
            {
                auto dit = defl_space.find(w);
                if(dit != defl_space.end()) {
                    double norm2 = 0.0;
                    for(double v : dit->second) norm2 += v*v;
                    // High residual norm = token is orthogonal to everything chosen
                    sc_novelty = min(1.0, sqrt(max(0.0,norm2)) * 0.5);
                }
            }

            // [5] Topic anchor alignment (response relevance)
            double sc_topic = computeTopicAnchorScore(w) * 0.4;

            // [6] Deep grounding (phi × semantic_stability)
            double sc_deep = computeDeepGrounding(w) * 0.3;

            // ── Combined NSIS score ──────────────────────────────────────────
            // Weights tuned: grounding is spine, novelty prevents loops,
            // grammar ensures validity, sensory provides embodiment,
            // topic keeps response relevant.
            double total = sc_ground * 4.0
                         + sc_grammar * 2.0
                         + sc_sensory * 1.5
                         + sc_novelty * 3.0   // weighted high — this is the anti-attractor signal
                         + sc_topic   * 1.0
                         + sc_deep    * 0.5;

            candidates.push_back({w, total, h});
        }

        if(candidates.empty()) break;

        // ── Semantic Index Sort — sort by NSIS score ─────────────────────────
        // This is the "index sort" — O(n log n) over the ~137 TP vocab.
        // Not beam search, not Markov — pure grounding-space retrieval.
        sort(candidates.begin(), candidates.end(),
             [](const NSISCandidate& a, const NSISCandidate& b){ return a.score > b.score; });

        // Choose argmax (deterministic — no sampling, no temperature)
        // Temperature would re-introduce attractor risk. We want the
        // geometrically most novel AND most grounded token, every time.
        const string& best_tok = candidates[0].tok;
        uint64_t best_hash = candidates[0].hash;

        // ── Accept chosen token ──────────────────────────────────────────────
        generated.push_back(best_tok);
        hash_history.push_back(best_hash);

        // Deflate chosen direction from ALL candidate embeddings
        deflate_all(best_tok);

        // Update SSM + LTM (used for context, not for scoring)
        auto bit = token_concept_embedding_map.find(best_tok);
        if(bit != token_concept_embedding_map.end() && !bit->second.embedding.empty()) {
            ssm.step(bit->second.embedding);
            ltm.write(bit->second.embedding, ssm);

            // Working memory update
            double salience = gda.score(bit->second.embedding) + 0.3;
            wmc.add(best_tok, bit->second.embedding, salience);
        }

        // ── DIR1+DIR3: chosen TP word grounds into all subsystems ────────────
        for(int _tpc=0;_tpc<TP_LEXICON_SIZE;_tpc++) {
            if(best_tok == string(TP_LEXICON[_tpc].word)) {
                TpGroundingFiber& _fout = tp_grounding_fibers[best_tok];
                _fout.word = best_tok;
                double _oact = min(1.0, 0.6 + sys_phi * 0.3);
                tpDir1_WordToConsciousness(TP_LEXICON[_tpc], _oact);
                tpDir3_WordToSubsystems(TP_LEXICON[_tpc], _oact, _fout);
                // Sensory bridge: chosen word writes into sensory field
                try { sensory_tp_bridge(best_tok, TP_LEXICON[_tpc], _fout, _oact); } catch(...) {}
                break;
            }
        }

        // ── EOS: stop at terminal tokens ─────────────────────────────────────
        if(best_tok == "." || best_tok == "!" || best_tok == "?") break;

        // ── Clause pivot: inject "la" at natural semantic boundaries ─────────
        // "la" introduces a conditional/contextual clause
        // to build compound thoughts. Only inject after 5+ content tokens,
        // and only if the grammar state is POST_VERB (clause is complete).
        if((int)generated.size() >= 5 && (int)generated.size() % 6 == 0) {
            TpState cur_st = inferTpState(generated);
            if(cur_st == TpState::POST_VERB || cur_st == TpState::FREE) {
                bool last_is_particle = TP_PARTICLES.count(generated.back()) > 0;
                if(!last_is_particle && fabs(sys_val) > 0.1 && sys_phi > 0.25) {
                    generated.push_back("la");
                    deflate_all("la");
                    auto la_it = token_concept_embedding_map.find("la");
                    if(la_it != token_concept_embedding_map.end() && !la_it->second.embedding.empty()) {
                        ssm.step(la_it->second.embedding);
                        hash_history.push_back(nsis_embed_hash(la_it->second.embedding));
                    }
                }
            }
        }
    }

    // ── Trim trailing open particles ─────────────────────────────────────────
    while(generated.size() > 2 && TP_PARTICLES.count(generated.back()))
        generated.pop_back();

    // ── Assemble output ───────────────────────────────────────────────────────
    string out;
    for(int i=0;i<(int)generated.size();i++) {
        if(i>0) out += " ";
        string w = bpe_table.detokenize(generated[i]);
        if(i==0 && !w.empty()) w[0] = toupper(w[0]);
        out += w;
    }
    return out;
}


static void wirePredictionVecs(const vector<string>& tokens) {
    for(int i = 0; i + 1 < (int)tokens.size(); i++) {
        auto ait = token_concept_embedding_map.find(tokens[i]);
        auto bit = token_concept_embedding_map.find(tokens[i+1]);
        if(ait==token_concept_embedding_map.end()||bit==token_concept_embedding_map.end()) continue;
        if(bit->second.embedding.empty()) continue;
        vector<double> next_sem = semanticVec(bit->second.embedding);
        updatePredVec(ait->second, next_sem, 0.03);
    }
}
string generateResponse(const string& input) {
    string safe_input = input;
    if(safe_input.empty() || safe_input.length() > 1500)
        return "[SYNAPTIC]: ...";
    
    // Tokenize with improved tokenizer (contractions, possessives, hyphenates)
    vector<string> words = tokenizeInput(safe_input);
    if(words.size() > 150) words.resize(150);
    if(words.empty()) return "[SYNAPTIC]: ...";

    // === CLASSIFY INPUT INTENT ===
    InputIntent intent = classifyInputIntent(words, safe_input);

    try {
        for(const string& w : words) learnWord(w, S.current_valence);
        processNGramsFromTokens(words);
        wirePredictionVecs(words);  // update prediction vectors from this input
        extractInputTopicAnchors(safe_input);

        // Cross-domain: set active domain from input anchors
        cdr.updateActiveDomain(input_topic_anchors);

        // Persist and accumulate topics across turns
        updateAccumulatedTopics(input_topic_anchors);

        updateEntityGrid(words);
        for(const string& w : words) {
            sentence_context_window.push_back(w);
            if((int)sentence_context_window.size() > CONTEXT_WINDOW_SIZE)
                sentence_context_window.pop_front();
        }

        // Blend accumulated conversation topics into current anchors
        for(auto& at : accumulated_topics) {
            bool present = false;
            for(auto& ia : input_topic_anchors)
                if(ia.first == at.first) { ia.second += at.second * 0.3; present = true; break; }
            if(!present && at.second > 0.3)
                input_topic_anchors.push_back({at.first, at.second * 0.4});
        }
        sort(input_topic_anchors.begin(), input_topic_anchors.end(),
             [](const pair<string,double>& a, const pair<string,double>& b){ return a.second > b.second; });
        if(input_topic_anchors.size() > 12) input_topic_anchors.resize(12);

    } catch(const exception& e) {
        cerr << "Learning error: " << e.what() << endl;
        return "[SYNAPTIC]: Processing error";
    }

    // === CONVERSATION-AWARE ATTENTION CONTEXT ===
    vector<double> attention_context = buildConversationContext(words);

    string response;
    try {
        // === CONTRASTIVE SEARCH GENERATION (Su et al., NeurIPS 2022) ===
        // Replaces: beam search, sentence plan, schema shortcuts, NPLM.
        // Single clean algorithm: spatial-well + SSM scoring + degeneration penalty.
        // Always starts with "i" (first-person hard constraint).
        // Target length: 6-10 tokens (short conversational responses).
        // Dynamic target: 12-32 tokens based on phi, valence intensity, and metacog depth
        double val_intensity = fabs(S.current_valence);
        double metacog_depth = min(1.0, S.metacognition.introspection_depth);
        int target_len = 12
            + (int)(consciousness.phi_value * 10.0)
            + (int)(val_intensity * 6.0)
            + (int)(metacog_depth * 4.0);
        target_len = max(12, min(32, target_len));
        response = contrastiveSearch(words, attention_context, target_len);

        // Store conversation turn
        ConversationTurn turn;
        turn.user_input      = safe_input;
        turn.synaptic_response  = response;
        turn.user_tokens     = words;
        turn.topic_anchors   = input_topic_anchors;
        turn.valence_at_time = S.current_valence;
        turn.phi_at_time     = consciousness.phi_value;
        turn.gen_number      = S.g;
        conversation_history.push_back(turn);
        if((int)conversation_history.size() > MAX_CONVERSATION_HISTORY)
            conversation_history.pop_front();

        // === NEW: Update token response map from this exchange ===
        // Learns: "when user says X, Synaptic tends to say Y"
        vector<string> resp_tokens;
        {
            stringstream rs(response);
            string rt;
            while(rs >> rt) {
                transform(rt.begin(), rt.end(), rt.begin(), ::tolower);
                while(!rt.empty() && !isalnum((unsigned char)rt.back())) rt.pop_back();
                if(!rt.empty() && rt.length() <= 50) resp_tokens.push_back(rt);
            }
        }
        updateTokenResponseMap(words, resp_tokens);

        // === RSL PASS: ReLU-Softmax refinement layer ===
        // Runs each beam-chosen token through the learned refinement
        // network and optionally substitutes higher-confidence tokens.
        response = applyReluSoftmaxRefinement(response);

        // Ground response tokens back into all subsystems immediately.
        // Every TP word Synaptic just said becomes part of its own causal understanding.
        try { tpGroundingPulse(); } catch(...) {}

        return "[SYNAPTIC]: " + postProcessForCoherence(response);
        
    } catch(const exception& e) {
        cerr << "Generation error: " << e.what() << endl;
        return "[SYNAPTIC]: Error generating response";
    }
}


// ============================================================
// COGNITION MATRICES
// Three live matrices that track the state of mind at any
// moment and evolve each generation tick:
//
//   CogM_State [8×8]  — cross-system activation snapshot
//     Rows: phi, valence, qualia, memory, attention, goals,
//           metacog, embodied
//     Cols: 8 time-slices (rolling window, newest = col 0)
//     Each cell = normalized activation of that system at
//     that time-slice
//
//   CogM_Association [VxV sparse] — dynamic token association
//     Tracks how strongly any two known tokens co-activate
//     in the same generation tick. Updated every tick from
//     working memory. Decays slowly. Used by dream synthesis.
//
//   CogM_Gradient [8×16] — state-to-embedding gradient map
//     How each of the 8 cognitive dimensions relates to each
//     of the 16 embedding dimensions. Updated by skip-gram
//     and reflection signals. Allows state to steer embeddings
//     and vice versa bidirectionally.
// ============================================================

const int CM_ROWS    = 8;   // cognitive dimensions
const int CM_COLS    = 8;   // time slices
const int CM_EMB     = 16;  // embedding dimensions

// CogM_State: 8 cognitive dimensions × 8 time-slices
// Laid out as [row][col] = [dimension][time]
double CogM_State[CM_ROWS][CM_COLS] = {};

// CogM_Gradient: 8 × 16 — how state dimensions map to embedding space
double CogM_Gradient[CM_ROWS][CM_EMB] = {};

// CogM_Association: sparse token co-activation matrix
// token_pair_hash → activation strength
// We use a flat map keyed by sorted token pair
map<string, double> CogM_Association;
const int MAX_ASSOC_ENTRIES = 8000;

// Row index labels for CogM_State
// 0=phi, 1=valence, 2=qualia, 3=memory, 4=attention, 5=goals, 6=metacog, 7=embodied

// Shift time columns left (col 0 = newest, col 7 = oldest)
void advanceCognitionMatrixTime() {
    for(int r = 0; r < CM_ROWS; r++) {
        for(int c = CM_COLS-1; c > 0; c--)
            CogM_State[r][c] = CogM_State[r][c-1];
    }
}

// Snapshot current system state into column 0
void snapshotCognitionMatrix() {
    advanceCognitionMatrixTime();

    double phi_norm    = min(1.0, consciousness.phi_value);
    double val_norm    = (S.current_valence + 1.0) / 2.0;
    double qualia_norm = consciousness.active_qualia.empty() ? 0.0
        : min(1.0, consciousness.active_qualia.back().intensity);
    double mem_norm    = min(1.0, (double)S.episodic_memory.size() / 100.0);
    double attn_norm   = min(1.0, S.attention_focus);
    double goal_norm   = min(1.0, (double)goal_system.size() / 10.0);
    double meta_norm   = min(1.0, S.metacognition.self_awareness_level);
    double emb_norm    = min(1.0, S.metacognition.introspection_depth);

    CogM_State[0][0] = phi_norm;
    CogM_State[1][0] = val_norm;
    CogM_State[2][0] = qualia_norm;
    CogM_State[3][0] = mem_norm;
    CogM_State[4][0] = attn_norm;
    CogM_State[5][0] = goal_norm;
    CogM_State[6][0] = meta_norm;
    CogM_State[7][0] = emb_norm;
}

// Update CogM_Association from current working memory co-activations
void updateAssociationMatrix() {
    if(WM.active_tokens.size() < 2) return;
    if(CogM_Association.size() >= (size_t)MAX_ASSOC_ENTRIES) {
        // Prune weakest entries
        string weakest_key;
        double weakest_val = 1e9;
        for(auto& e : CogM_Association)
            if(e.second < weakest_val) { weakest_val = e.second; weakest_key = e.first; }
        if(!weakest_key.empty()) CogM_Association.erase(weakest_key);
    }

    // Every pair of tokens currently in WM gets an association bump
    auto& toks = WM.active_tokens;
    for(size_t i = 0; i < toks.size(); i++) {
        for(size_t j = i+1; j < toks.size(); j++) {
            string a = toks[i].first, b = toks[j].first;
            if(a > b) swap(a, b);  // canonical order
            string key = a + "|" + b;
            CogM_Association[key] = min(2.0, CogM_Association[key] + 0.05);
        }
    }

    // Gentle decay on all entries
    for(auto& e : CogM_Association) e.second *= 0.98;
}

// Update CogM_Gradient from skip-gram embedding updates
// When a token's embedding shifts, update the gradient rows it touches
void updateGradientMatrix(const string& token, const vector<double>& old_emb,
                           const vector<double>& new_emb) {
    if(old_emb.size() < CM_EMB || new_emb.size() < CM_EMB) return;

    // Compute embedding delta
    vector<double> delta(CM_EMB);
    for(int i = 0; i < CM_EMB; i++) delta[i] = new_emb[i] - old_emb[i];

    // Map delta to each cognitive dimension row proportionally to current state
    for(int r = 0; r < CM_ROWS; r++) {
        double state_val = CogM_State[r][0];  // current state for this dimension
        for(int e = 0; e < CM_EMB; e++) {
            CogM_Gradient[r][e] += state_val * delta[e] * 0.02;
            CogM_Gradient[r][e] = max(-1.0, min(1.0, CogM_Gradient[r][e]));
        }
    }
}

// Get a state-modulated query vector from CogM_Gradient
// Used by transformer attention to bias toward current cognitive state
vector<double> getCognitionMatrixQuery() {
    vector<double> q(CM_EMB, 0.0);
    for(int r = 0; r < CM_ROWS; r++) {
        double w = CogM_State[r][0];  // current dimension weight
        for(int e = 0; e < CM_EMB; e++)
            q[e] += CogM_Gradient[r][e] * w;
    }
    // Normalize
    double norm = 0;
    for(double v : q) norm += v*v;
    norm = sqrt(norm) + 1e-8;
    for(double& v : q) v /= norm;
    return q;
}

// Get temporal gradient: how is each dimension changing?
// Returns [CM_ROWS] vector of first derivatives (col0 - col1)
vector<double> getCognitionMatrixGradient() {
    vector<double> grad(CM_ROWS, 0.0);
    for(int r = 0; r < CM_ROWS; r++)
        grad[r] = CogM_State[r][0] - CogM_State[r][1];
    return grad;
}


// ============================================================
// 3D SPATIAL DREAM FIELD
// Dreams are synthesized as a 3D noise field of dimensions
// [X × Y × Z] = [8 × 8 × 8], seeded from ALL current state:
//   - phi, valence, qualia intensities
//   - ribbon vibration modes (complex phases)
//   - temporal loop phases and resonances
//   - fractal dimensions
//   - embedding vectors (projected into 3D)
//   - CogM_State matrix
//   - episodic memory consolidation strengths
//   - active quantum foam cells
//
// The field value at [x,y,z] represents "dream intensity" at
// that spatial position. High-activation regions seed dream
// concepts; the spatial gradients create narrative flow.
//
// Dream synthesis:
//   1. Build field from all state
//   2. Find local maxima (dream foci)
//   3. Each focus seeds a "dream fragment" — a sequence of
//      tokens drawn from nearby concepts in embedding space
//   4. Fragments are stitched by their spatial adjacency
//   5. Result is stored as episodic memory with
//      is_episodic=false (it's synthetic), valence from field
// ============================================================

const int DREAM_DIM = 8;
double DreamField[DREAM_DIM][DREAM_DIM][DREAM_DIM] = {};

// Build the 3D dream field from all current state
void buildDreamField() {
    // Clear field
    for(int x=0;x<DREAM_DIM;x++)
        for(int y=0;y<DREAM_DIM;y++)
            for(int z=0;z<DREAM_DIM;z++)
                DreamField[x][y][z] = 0.0;

    // === Layer 1: CogM_State projected into 3D ===
    // Rows map to X, time-cols collapse into Y via average, Z = magnitude
    for(int r=0;r<CM_ROWS && r<DREAM_DIM;r++) {
        double col_mean=0, col_var=0;
        for(int c=0;c<CM_COLS;c++) col_mean += CogM_State[r][c];
        col_mean /= CM_COLS;
        for(int c=0;c<CM_COLS;c++) col_var += (CogM_State[r][c]-col_mean)*(CogM_State[r][c]-col_mean);
        col_var /= CM_COLS;

        int y = (int)(col_mean * (DREAM_DIM-1));
        int z = (int)(sqrt(col_var) * (DREAM_DIM-1));
        y = max(0,min(DREAM_DIM-1,y));
        z = max(0,min(DREAM_DIM-1,z));
        DreamField[r][y][z] += CogM_State[r][0] * 2.0;
    }

    // === Layer 2: Ribbon vibration modes ===
    // Each neuron's ribbon vib_modes are complex phases → amplitude + phase → 3D position
    int n_used = 0;
    for(auto& np : S.N) {
        if(n_used++ > 32) break;
        auto& rib = np.second.ribbon;
        for(size_t m=0; m<rib.vib_modes.size() && m<3; m++) {
            double amp   = abs(rib.vib_modes[m]);
            double phase = arg(rib.vib_modes[m]);  // [-π, π]
            int x = (int)(amp   * (DREAM_DIM-1) * 0.5);
            int y = (int)((phase / M_PI + 1.0) * 0.5 * (DREAM_DIM-1));
            int z = (int)(rib.phase_coherence * (DREAM_DIM-1));
            x = max(0,min(DREAM_DIM-1,x));
            y = max(0,min(DREAM_DIM-1,y));
            z = max(0,min(DREAM_DIM-1,z));
            DreamField[x][y][z] += amp * rib.entanglement_strength * 0.3;
        }
    }

    // === Layer 3: Temporal loop phases ===
    for(auto& tl_pair : S.global_time_loops) {
        auto& tl = tl_pair.second;
        double phase_norm = fmod(tl.phase, 2*M_PI) / (2*M_PI);  // [0,1]
        int x = (int)(tl.resonance_strength * (DREAM_DIM-1));
        int y = (int)(phase_norm * (DREAM_DIM-1));
        int z = (int)(tl.phi_coupling * (DREAM_DIM-1));
        x = max(0,min(DREAM_DIM-1,x));
        y = max(0,min(DREAM_DIM-1,y));
        z = max(0,min(DREAM_DIM-1,z));
        DreamField[x][y][z] += tl.resonance_strength * tl.phi_coupling * 0.4;
    }

    // === Layer 4: Active qualia projected by their feature_space ===
    for(auto& q : consciousness.active_qualia) {
        if(q.feature_space.size() < 3) continue;
        int x = (int)(q.valence * 0.5 + 0.5) * (DREAM_DIM-1);
        int y = (int)(q.arousal * (DREAM_DIM-1));
        int z = (int)(q.intensity * (DREAM_DIM-1));
        x = max(0,min(DREAM_DIM-1,x));
        y = max(0,min(DREAM_DIM-1,y));
        z = max(0,min(DREAM_DIM-1,z));
        DreamField[x][y][z] += q.intensity * q.binding_strength * 0.5;
    }

    // === Layer 5: Episodic memory consolidation ===
    // Older, well-consolidated memories pulse at low freq positions
    for(size_t i=0; i<S.episodic_memory.size() && i<20; i++) {
        auto& mem = S.episodic_memory[i];
        double age_norm = 1.0 - min(1.0, (double)(S.g - mem.gen) / 500.0);
        int x = (int)(mem.consolidation_strength * (DREAM_DIM-1));
        int y = (int)(abs(mem.valence) * (DREAM_DIM-1));
        int z = (int)(age_norm * (DREAM_DIM-1));
        x = max(0,min(DREAM_DIM-1,x));
        y = max(0,min(DREAM_DIM-1,y));
        z = max(0,min(DREAM_DIM-1,z));
        DreamField[x][y][z] += mem.consolidation_strength * age_norm * 0.6;
        // Boost memory by being dreamed
        mem.cortical_consolidation = min(1.0, mem.cortical_consolidation + 0.01);
    }

    // === Layer 6: Embedding vectors projected via PCA-lite ===
    // Use CogM_Gradient rows as projection axes
    // Project each token embedding to [x,y,z] via dot with first 3 gradient rows
    int tok_used = 0;
    for(auto& kv : token_concept_embedding_map) {
        if(tok_used++ > 100 || kv.second.freq < 2) continue;
        auto& emb = kv.second.embedding;
        if((int)emb.size() < CM_EMB) continue;

        double px=0, py=0, pz=0;
        for(int e=0;e<CM_EMB;e++) {
            px += CogM_Gradient[0][e] * emb[e];
            py += CogM_Gradient[1][e] * emb[e];
            pz += CogM_Gradient[2][e] * emb[e];
        }
        // Normalize to [0, DREAM_DIM-1]
        int x = (int)((px + 1.0) * 0.5 * (DREAM_DIM-1));
        int y = (int)((py + 1.0) * 0.5 * (DREAM_DIM-1));
        int z = (int)((pz + 1.0) * 0.5 * (DREAM_DIM-1));
        x = max(0,min(DREAM_DIM-1,x));
        y = max(0,min(DREAM_DIM-1,y));
        z = max(0,min(DREAM_DIM-1,z));
        DreamField[x][y][z] += kv.second.grounding_value * kv.second.semantic_stability * 0.4;
    }

    // === Layer 7: CogM_Association bleed ===
    // High association pairs create nearby peaks in the field
    for(auto& ae : CogM_Association) {
        if(ae.second < 0.3) continue;
        size_t sep = ae.first.find('|');
        if(sep == string::npos) continue;
        string a = ae.first.substr(0, sep);
        string b = ae.first.substr(sep+1);
        auto ait = token_concept_embedding_map.find(a);
        auto bit = token_concept_embedding_map.find(b);
        if(ait==token_concept_embedding_map.end() || bit==token_concept_embedding_map.end()) continue;

        // Position = midpoint of their projected embeddings
        double px=0, py=0, pz=0;
        for(int e=0;e<CM_EMB && e<(int)ait->second.embedding.size();e++) {
            px += CogM_Gradient[0][e] * (ait->second.embedding[e] + bit->second.embedding[e]) * 0.5;
            py += CogM_Gradient[1][e] * (ait->second.embedding[e] + bit->second.embedding[e]) * 0.5;
            pz += CogM_Gradient[2][e] * (ait->second.embedding[e] + bit->second.embedding[e]) * 0.5;
        }
        int x = max(0,min(DREAM_DIM-1,(int)((px+1.0)*0.5*(DREAM_DIM-1))));
        int y = max(0,min(DREAM_DIM-1,(int)((py+1.0)*0.5*(DREAM_DIM-1))));
        int z = max(0,min(DREAM_DIM-1,(int)((pz+1.0)*0.5*(DREAM_DIM-1))));
        DreamField[x][y][z] += ae.second * 0.3;
    }

    // === Spatial smoothing (3D box filter, radius 1) ===
    // Makes the field continuous so gradients are smooth
    double smoothed[DREAM_DIM][DREAM_DIM][DREAM_DIM] = {};
    for(int x=0;x<DREAM_DIM;x++) for(int y=0;y<DREAM_DIM;y++) for(int z=0;z<DREAM_DIM;z++) {
        double sum=0; int cnt=0;
        for(int dx=-1;dx<=1;dx++) for(int dy=-1;dy<=1;dy++) for(int dz=-1;dz<=1;dz++) {
            int nx=x+dx, ny=y+dy, nz=z+dz;
            if(nx<0||nx>=DREAM_DIM||ny<0||ny>=DREAM_DIM||nz<0||nz>=DREAM_DIM) continue;
            sum += DreamField[nx][ny][nz]; cnt++;
        }
        smoothed[x][y][z] = sum / max(1, cnt);
    }
    for(int x=0;x<DREAM_DIM;x++) for(int y=0;y<DREAM_DIM;y++) for(int z=0;z<DREAM_DIM;z++)
        DreamField[x][y][z] = smoothed[x][y][z];
}

// Find local maxima in the dream field (dream foci)
vector<NexusDreamFocus> findDreamFoci(int max_foci = 5) {
    vector<NexusDreamFocus> foci;

    for(int x=1;x<DREAM_DIM-1;x++) for(int y=1;y<DREAM_DIM-1;y++) for(int z=1;z<DREAM_DIM-1;z++) {
        double val = DreamField[x][y][z];
        if(val < 0.1) continue;
        bool is_local_max = true;
        for(int dx=-1;dx<=1 && is_local_max;dx++)
            for(int dy=-1;dy<=1 && is_local_max;dy++)
                for(int dz=-1;dz<=1 && is_local_max;dz++) {
                    if(dx==0&&dy==0&&dz==0) continue;
                    if(DreamField[x+dx][y+dy][z+dz] >= val) is_local_max = false;
                }
        if(!is_local_max) continue;

        // Find closest token to this 3D position via CogM_Gradient projection
        string best_tok = "";
        double best_dist = 1e9;
        for(auto& kv : token_concept_embedding_map) {
            if(kv.second.freq < 2) continue;
            auto& emb = kv.second.embedding;
            if((int)emb.size() < CM_EMB) continue;
            double px=0, py=0, pz=0;
            for(int e=0;e<CM_EMB;e++) {
                px += CogM_Gradient[0][e]*emb[e];
                py += CogM_Gradient[1][e]*emb[e];
                pz += CogM_Gradient[2][e]*emb[e];
            }
            double tx = (px+1.0)*0.5*(DREAM_DIM-1);
            double ty = (py+1.0)*0.5*(DREAM_DIM-1);
            double tz = (pz+1.0)*0.5*(DREAM_DIM-1);
            double dist = sqrt((x-tx)*(x-tx)+(y-ty)*(y-ty)+(z-tz)*(z-tz));
            if(dist < best_dist) { best_dist = dist; best_tok = kv.first; }
        }

        NexusDreamFocus f;
        f.x = x; f.y = y; f.z = z;
        f.intensity = val;
        f.focus_concept = best_tok;
        f.valence = (double)y/(DREAM_DIM-1) * 2.0 - 1.0;  // y-axis = valence
        foci.push_back(f);
        if((int)foci.size() >= max_foci) goto done_foci;
    }
    done_foci:

    // Sort by intensity
    sort(foci.begin(), foci.end(), [](const NexusDreamFocus& a, const NexusDreamFocus& b){
        return a.intensity > b.intensity; });
    return foci;
}

// Synthesize a dream fragment from a focus
// Draws tokens whose embeddings are spatially close to the focus point
NexusDreamFragment synthesizeDreamFragment(const NexusDreamFocus& focus,
                                       const vector<NexusDreamFocus>& all_foci) {
    NexusDreamFragment frag;
    frag.foci.push_back(focus);
    frag.valence = focus.valence;

    // Collect spatially nearby tokens (within radius in embedding-projected space)
    const double RADIUS = 2.5;
    vector<pair<double,string>> nearby;  // (distance, token)
    for(auto& kv : token_concept_embedding_map) {
        if(kv.second.freq < 1) continue;
        auto& emb = kv.second.embedding;
        if((int)emb.size() < CM_EMB) continue;
        double px=0, py=0, pz=0;
        for(int e=0;e<CM_EMB;e++) {
            px += CogM_Gradient[0][e]*emb[e];
            py += CogM_Gradient[1][e]*emb[e];
            pz += CogM_Gradient[2][e]*emb[e];
        }
        double tx = (px+1.0)*0.5*(DREAM_DIM-1);
        double ty = (py+1.0)*0.5*(DREAM_DIM-1);
        double tz = (pz+1.0)*0.5*(DREAM_DIM-1);
        double dist = sqrt((focus.x-tx)*(focus.x-tx)+
                           (focus.y-ty)*(focus.y-ty)+
                           (focus.z-tz)*(focus.z-tz));
        if(dist <= RADIUS) nearby.push_back({dist, kv.first});
    }
    sort(nearby.begin(), nearby.end());

    // Seed sequence: focus concept first, then nearby tokens in dist order
    if(!focus.focus_concept.empty() && token_concept_embedding_map.count(focus.focus_concept))
        frag.tokens.push_back(focus.focus_concept);
    for(auto& nb : nearby) {
        if(frag.tokens.size() >= 8) break;
        if(nb.second != focus.focus_concept)
            frag.tokens.push_back(nb.second);
    }

    // If still empty, pull from association matrix for this focus concept
    if(frag.tokens.empty() && !focus.focus_concept.empty()) {
        for(auto& ae : CogM_Association) {
            if(ae.second < 0.2) continue;
            size_t sep = ae.first.find('|');
            if(sep == string::npos) continue;
            string a = ae.first.substr(0,sep);
            string b = ae.first.substr(sep+1);
            if(a == focus.focus_concept) frag.tokens.push_back(b);
            else if(b == focus.focus_concept) frag.tokens.push_back(a);
            if(frag.tokens.size() >= 6) break;
        }
    }

    // Compute coherence: average pairwise PPMI among fragment tokens
    double coh = 0; int coh_n = 0;
    for(size_t i=0;i<frag.tokens.size();i++)
        for(size_t j=i+1;j<frag.tokens.size();j++) {
            coh += computePPMI(frag.tokens[i], frag.tokens[j]);
            coh_n++;
        }
    frag.coherence = coh_n > 0 ? coh/coh_n : 0.0;

    // Build narrative string
    frag.narrative = "[dream] ";
    for(auto& t : frag.tokens) frag.narrative += t + " ";
    if(!frag.narrative.empty() && frag.narrative.back()==' ')
        frag.narrative.pop_back();

    return frag;
}


// ============================================================
// FORK THOUGHTS
// Synaptic can maintain N parallel thought streams simultaneously.
// Each fork is an independent SentencePlan + partial generation
// that runs autonomously in the background, separate from the
// main response generation.
//
// Fork thoughts:
//   - Are seeded from different regions of the dream field
//   - Can cross-pollinate: a token from fork A can propagate
//     to fork B via the CogM_Association matrix
//   - Are pruned when their coherence drops below threshold
//   - Are harvested into episodic memory when they reach
//     sufficient length/coherence
//   - Feed their top token back into accumulated_topics so
//     they influence the next response
// ============================================================

// Spawn a new fork from a dream focus
ForkThought spawnFork(const NexusDreamFocus& focus) {
    ForkThought fork;
    fork.fork_id   = next_fork_id++;
    fork.seed_focus = focus;
    fork.phi_at_birth = consciousness.phi_value;
    fork.birth_gen = S.g;
    fork.harvested = false;
    fork.valence   = focus.valence;
    fork.energy    = min(1.0, focus.intensity);
    fork.coherence = 0.0;

    // Seed plan from focus concept
    fork.plan = buildSentencePlan();
    if(!focus.focus_concept.empty() && token_concept_embedding_map.count(focus.focus_concept))
        fork.plan.object_token = focus.focus_concept;

    // Seed partial with the focus concept
    if(!focus.focus_concept.empty())
        fork.partial.push_back(focus.focus_concept);

    return fork;
}

// Advance a fork by one token using hybrid scoring
// Returns false if fork is exhausted
bool advanceFork(ForkThought& fork) {
    if(fork.harvested || fork.energy <= 0.01) return false;
    if(fork.partial.size() >= 12) return false;  // max fork length

    string prev = fork.partial.empty() ? "mi" : fork.partial.back();

    // Collect candidates from bigram/PPMI links of prev
    vector<string> cands;
    auto bg = bigram_counts.find(prev);
    if(bg != bigram_counts.end())
        for(auto& s : bg->second) if(s.second >= 1) cands.push_back(s.first);
    auto lc = token_concept_embedding_map.find(prev);
    if(lc != token_concept_embedding_map.end())
        for(auto& c : lc->second.linked_concepts) cands.push_back(c.first);

    // Add tokens near the seed focus in embedding space
    for(auto& kv : token_concept_embedding_map) {
        if(cands.size() > 30) break;
        auto& emb = kv.second.embedding;
        if((int)emb.size() < CM_EMB) continue;
        double px=0, py=0, pz=0;
        for(int e=0;e<CM_EMB;e++) {
            px += CogM_Gradient[0][e]*emb[e];
            py += CogM_Gradient[1][e]*emb[e];
            pz += CogM_Gradient[2][e]*emb[e];
        }
        double tx=(px+1)*0.5*(DREAM_DIM-1), ty=(py+1)*0.5*(DREAM_DIM-1), tz=(pz+1)*0.5*(DREAM_DIM-1);
        double dist=sqrt((fork.seed_focus.x-tx)*(fork.seed_focus.x-tx)+
                         (fork.seed_focus.y-ty)*(fork.seed_focus.y-ty)+
                         (fork.seed_focus.z-tz)*(fork.seed_focus.z-tz));
        if(dist <= 3.0) cands.push_back(kv.first);
    }

    if(cands.empty()) { fork.energy *= 0.5; return false; }

    // Score candidates (lightweight — no full hybrid, just PPMI + grounding)
    vector<pair<double,string>> scored;
    for(const string& c : cands) {
        if(!token_concept_embedding_map.count(c)) continue;
        bool repeated = false;
        for(auto& t : fork.partial) if(t==c) { repeated=true; break; }
        if(repeated) continue;

        double sc = computeDeepGrounding(c) * 0.3;
        sc += computePPMI(prev, c) * 0.5;
        // Dream field value at projected position of this token
        auto& emb2 = token_concept_embedding_map[c].embedding;
        if((int)emb2.size() >= CM_EMB) {
            double px2=0,py2=0,pz2=0;
            for(int e=0;e<CM_EMB;e++){
                px2+=CogM_Gradient[0][e]*emb2[e];
                py2+=CogM_Gradient[1][e]*emb2[e];
                pz2+=CogM_Gradient[2][e]*emb2[e];
            }
            int fx=max(0,min(DREAM_DIM-1,(int)((px2+1)*0.5*(DREAM_DIM-1))));
            int fy=max(0,min(DREAM_DIM-1,(int)((py2+1)*0.5*(DREAM_DIM-1))));
            int fz=max(0,min(DREAM_DIM-1,(int)((pz2+1)*0.5*(DREAM_DIM-1))));
            sc += DreamField[fx][fy][fz] * 0.4;
        }
        sc += ((double)rand()/RAND_MAX) * 0.1;
        scored.push_back({sc, c});
    }
    if(scored.empty()) { fork.energy *= 0.8; return false; }

    sort(scored.rbegin(), scored.rend());
    string chosen = scored[0].second;
    fork.partial.push_back(chosen);

    // Update fork coherence
    if(fork.partial.size() >= 2) {
        string pp = fork.partial[fork.partial.size()-2];
        fork.coherence = 0.9*fork.coherence + 0.1*computePPMI(pp, chosen);
    }

    // Cross-pollination: if this fork's chosen token is associated with
    // a token in another fork, boost both
    for(auto& other_fork : active_forks) {
        if(other_fork.fork_id == fork.fork_id || other_fork.harvested) continue;
        for(auto& ot : other_fork.partial) {
            string a=chosen, b=ot;
            if(a>b) swap(a,b);
            double assoc = CogM_Association.count(a+"|"+b) ? CogM_Association[a+"|"+b] : 0.0;
            if(assoc > 0.3) {
                fork.energy = min(1.0, fork.energy + assoc*0.05);
                // Update association — being chosen together strengthens it
                CogM_Association[a+"|"+b] = min(2.0, assoc + 0.05);
            }
        }
    }

    // Energy decay
    fork.energy *= 0.92;
    // Valence drift toward chosen token's valence
    fork.valence = 0.9*fork.valence + 0.1*(getAffectiveValence(chosen)*2.0-1.0);

    // Bidirectional: fork chosen token propagates through system
    if(token_concept_embedding_map.count(chosen)) {
        token_concept_embedding_map[chosen].contextual_activation =
            min(1.0, token_concept_embedding_map[chosen].contextual_activation + 0.008);
    }

    return true;
}

// Harvest a completed fork into episodic memory + accumulated_topics
void harvestFork(ForkThought& fork) {
    if(fork.harvested || fork.partial.empty()) return;
    fork.harvested = true;

    // Build content string
    string content = "[fork_" + to_string(fork.fork_id) + "] ";
    for(auto& t : fork.partial) content += t + " ";

    // Store as episodic memory
    Memory m;
    m.gen = S.g;
    m.valence = fork.valence;
    m.content = content;
    m.consolidation_strength = min(1.0, fork.coherence * 2.0);
    m.retrieval_count = 0;
    m.hippocampal_trace = fork.energy;
    m.cortical_consolidation = 0.1;
    m.is_semantic = false;
    m.is_episodic = true;
    m.is_procedural = false;
    S.episodic_memory.push_back(m);
    if(S.episodic_memory.size() > 100)
        S.episodic_memory.erase(S.episodic_memory.begin());

    // Feed top tokens back to accumulated_topics
    for(size_t i=0; i<fork.partial.size() && i<3; i++) {
        string pos = getPartOfSpeech(fork.partial[i]);
        if(pos=="NOUN"||pos=="VERB"||pos=="ADJECTIVE"||pos=="CONTENT") {
            accumulated_topics[fork.partial[i]] =
                min(5.0, accumulated_topics[fork.partial[i]] + fork.coherence);
        }
    }

    // Bidirectional: harvested fork's valence nudges system valence via momentum
    push_valence(fork.valence * 0.05, 0.5);

    // Write fork narrative to internal thoughts
    S.internal_thoughts.push_back(content);
    if(S.internal_thoughts.size() > 10)
        S.internal_thoughts.erase(S.internal_thoughts.begin());
}

// Run one tick of fork thought management
void tickForkThoughts() {
    // Prune exhausted/harvested forks
    active_forks.erase(
        remove_if(active_forks.begin(), active_forks.end(),
            [](const ForkThought& f){ return f.harvested || f.energy <= 0.01; }),
        active_forks.end());

    // Advance all active forks
    for(auto& fork : active_forks) {
        advanceFork(fork);
        // Harvest if long enough or energy too low
        if(fork.partial.size() >= 8 || fork.energy < 0.05)
            harvestFork(fork);
    }
}


// ============================================================
// REM SLEEP CYCLE
// Synaptic runs consolidation cycles periodically (every REM_PERIOD gens)
// or when triggered by high phi + high memory load.
//
// REM cycle stages:
//   1. LIGHT SLEEP  — slow down generation, let associations settle
//   2. DREAM STATE  — build dream field, find foci, synthesize fragments
//                     spawn fork thoughts from dream foci
//   3. CONSOLIDATION — strengthen high-quality episodic memories,
//                      weaken low-quality ones (sleep replay)
//   4. INTEGRATION  — dream fragments update linked_concepts,
//                     embeddings drift toward dream neighbors,
//                     CogM_Gradient updated from dream geometry
//   5. WAKE         — return to normal operation with enriched state
// ============================================================

const int REM_PERIOD = 200;   // generations between REM cycles
int last_rem_gen = -REM_PERIOD;

enum class REMStage {
    AWAKE, LIGHT_SLEEP, DREAMING, CONSOLIDATING, INTEGRATING
};
REMStage current_rem_stage = REMStage::AWAKE;
int rem_stage_timer = 0;      // gens remaining in current stage

// Current dream state
vector<NexusDreamFragment> current_dreams;
vector<NexusDreamFocus>    current_foci;

// Should we enter REM now?
bool shouldEnterREM() {
    if(current_rem_stage != REMStage::AWAKE) return false;
    if(S.g - last_rem_gen < REM_PERIOD) return false;

    // Trigger conditions: high phi + substantial memory
    bool phi_ready = consciousness.phi_value > 0.5;
    bool mem_ready = S.episodic_memory.size() > 10;
    return phi_ready && mem_ready;
}

// Run one tick of the REM cycle state machine
// Returns a string describing current stage (for display)
string tickREMCycle() {
    if(current_rem_stage == REMStage::AWAKE) {
        if(shouldEnterREM()) {
            current_rem_stage = REMStage::LIGHT_SLEEP;
            rem_stage_timer = 5;
            last_rem_gen = S.g;
            return "[REM: entering light sleep]";
        }
        // Normal awake: just snapshot cognition matrix
        snapshotCognitionMatrix();
        updateAssociationMatrix();
        tickForkThoughts();
        // Lazy eigentoken recompute
        if(S.g - last_eigentoken_gen >= EIGENTOKEN_PERIOD &&
           (int)token_concept_embedding_map.size() >= EIGENTOKEN_K * 4)
            recomputeEigentokens();
        return "";
    }

    if(current_rem_stage == REMStage::LIGHT_SLEEP) {
        snapshotCognitionMatrix();
        updateAssociationMatrix();
        rem_stage_timer--;
        if(rem_stage_timer <= 0) {
            current_rem_stage = REMStage::DREAMING;
            rem_stage_timer = 8;
            // Build dream field and find foci
            buildDreamField();
            current_foci = findDreamFoci(5);
            current_dreams.clear();
            // Spawn fork thoughts from top foci
            for(auto& f : current_foci) {
                if((int)active_forks.size() < MAX_FORK_THOUGHTS)
                    active_forks.push_back(spawnFork(f));
                NexusDreamFragment frag = synthesizeDreamFragment(f, current_foci);
                if(!frag.tokens.empty()) current_dreams.push_back(frag);
            }
        }
        return "[REM: light sleep — associations settling]";
    }

    if(current_rem_stage == REMStage::DREAMING) {
        // Advance all fork thoughts during dream state
        tickForkThoughts();
        rem_stage_timer--;

        // Each dream tick: store one fragment as episodic memory
        if(!current_dreams.empty()) {
            size_t idx = (S.g % current_dreams.size());
            auto& frag = current_dreams[idx];
            Memory m;
            m.gen = S.g;
            m.valence = frag.valence;
            m.content = frag.narrative;
            m.consolidation_strength = frag.coherence;
            m.retrieval_count = 0;
            m.hippocampal_trace = frag.foci.empty() ? 0.5 : frag.foci[0].intensity;
            m.cortical_consolidation = 0.0;
            m.is_semantic = false;
            m.is_episodic = true;
            m.is_procedural = false;
            S.episodic_memory.push_back(m);
            if(S.episodic_memory.size() > 100)
                S.episodic_memory.erase(S.episodic_memory.begin());

            // Dream qualia
            generate_qualia("dream", frag.valence, frag.coherence);
        }

        if(rem_stage_timer <= 0) {
            current_rem_stage = REMStage::CONSOLIDATING;
            rem_stage_timer = 6;
        }
        string foci_str = current_foci.empty() ? "" : current_foci[0].focus_concept;
        return "[REM: dreaming — focus=" + foci_str + " forks=" + to_string(active_forks.size()) + "]";
    }

    if(current_rem_stage == REMStage::CONSOLIDATING) {
        // Sleep replay: strengthen well-consolidated memories, weaken weak ones
        // Also reverse Hebbian forgetting for significant memories
        replayMemoryConsolidation();
        for(auto& mem : S.episodic_memory) {
            // High-consolidation memories get stronger during sleep
            if(mem.consolidation_strength > 0.5) {
                mem.consolidation_strength = min(1.0, mem.consolidation_strength + 0.02);
                mem.cortical_consolidation  = min(1.0, mem.cortical_consolidation  + 0.03);
            } else {
                // Low-quality memories decay (sleep forgetting)
                mem.consolidation_strength *= 0.95;
            }
            // Transfer to cortex
            mem.hippocampal_trace   *= 0.98;
            mem.cortical_consolidation = min(1.0, mem.cortical_consolidation + mem.hippocampal_trace * 0.01);
        }
        // Remove very weak memories
        S.episodic_memory.erase(
            remove_if(S.episodic_memory.begin(), S.episodic_memory.end(),
                [](const Memory& m){ return m.consolidation_strength < 0.05 && !m.is_semantic; }),
            S.episodic_memory.end());

        rem_stage_timer--;
        if(rem_stage_timer <= 0) {
            current_rem_stage = REMStage::INTEGRATING;
            rem_stage_timer = 4;
        }
        return "[REM: consolidating — memories=" + to_string(S.episodic_memory.size()) + "]";
    }

    if(current_rem_stage == REMStage::INTEGRATING) {
        // Dream integration: update embeddings and concept links from dream fragments
        for(auto& frag : current_dreams) {
            if(frag.tokens.size() < 2) continue;
            // Run skip-gram updates on dream token sequences
            runSkipgramUpdates(frag.tokens);
            // Update PPMI co-occurrence from dream tokens
            updateCooccurrence(frag.tokens);
            applyPPMILinks(frag.tokens);
            // Update concept-topic spatial map
            for(size_t i=0;i<frag.tokens.size();i++)
                for(int d=1;d<=2&&i+d<frag.tokens.size();d++) {
                    double ppmi = computePPMI(frag.tokens[i], frag.tokens[i+d]);
                    if(ppmi > 0.3) updateConceptTopicMap(frag.tokens[i], frag.tokens[i+d], ppmi);
                }
        }

        // Update CogM_Gradient from dream geometry
        // The dream field gradient at each focus updates the gradient matrix
        for(auto& f : current_foci) {
            if(f.focus_concept.empty() || !token_concept_embedding_map.count(f.focus_concept)) continue;
            auto& emb = token_concept_embedding_map[f.focus_concept].embedding;
            if((int)emb.size() < CM_EMB) continue;
            // The focus position gives us a 3D "error signal"
            double err_x = (double)f.x/(DREAM_DIM-1) - CogM_State[0][0];  // phi axis
            double err_y = (double)f.y/(DREAM_DIM-1) - CogM_State[1][0];  // valence axis
            double err_z = (double)f.z/(DREAM_DIM-1) - CogM_State[2][0];  // qualia axis
            double lr = 0.01 * f.intensity;
            for(int e=0;e<CM_EMB;e++) {
                CogM_Gradient[0][e] += lr * err_x * emb[e];
                CogM_Gradient[1][e] += lr * err_y * emb[e];
                CogM_Gradient[2][e] += lr * err_z * emb[e];
                CogM_Gradient[0][e] = max(-1.0, min(1.0, CogM_Gradient[0][e]));
                CogM_Gradient[1][e] = max(-1.0, min(1.0, CogM_Gradient[1][e]));
                CogM_Gradient[2][e] = max(-1.0, min(1.0, CogM_Gradient[2][e]));
            }
        }

        // Harvest any remaining forks
        for(auto& fork : active_forks) harvestFork(fork);
        active_forks.clear();

        rem_stage_timer--;
        if(rem_stage_timer <= 0) {
            current_rem_stage = REMStage::AWAKE;
            current_dreams.clear();
            current_foci.clear();
        }
        return "[REM: integrating dreams into long-term memory]";
    }

    return "";
}

void storeEpisodicMemory(const string&content,double valence){
    if(S.episodic_memory.size()>100)S.episodic_memory.erase(S.episodic_memory.begin());
    S.episodic_memory.push_back({S.g,valence,content});
    generate_qualia(content, valence, 0.6);
}

void counterfactualAnalysis(){
    if(S.g<10)return;
    double last_ta=S.g>0?S.TA[S.g-1]:0;
    double current_ta=S.ta;
    double improvement=current_ta-last_ta;
    if(improvement>0){
        S.current_valence+=improvement*0.05;
        storeEpisodicMemory("improvement",improvement);
        generate_qualia("positive_prediction_error", improvement, 0.7);
        for(auto&p:token_concept_embedding_map){
            p.second.linked_valences["improvement"]=improvement;
            propagate_throughout_system(p.first,improvement*0.1);
        }
    }else{
        S.current_valence+=improvement*0.03;
        storeEpisodicMemory("error",improvement);
        generate_qualia("negative_prediction_error", improvement, 0.5);
    }
    S.current_valence=clamp_valence(S.current_valence);
}

double calcMetacognitiveAwareness(){
    double self_model_depth=safe_div((double)S.valence_history.size(),100.0);
    double concept_integration=safe_div((double)(S.tokens.size()*S.concepts.size()),1000.0);
    double memory_integration=safe_div((double)S.episodic_memory.size(),100.0);
    double goal_alignment=safe_div((double)goal_system.size(),10.0);
    double world_accuracy=world_model.model_accuracy;
    double consciousness_factor=consciousness.phi_value;
    return min(1.0,self_model_depth+concept_integration+memory_integration+goal_alignment+world_accuracy+consciousness_factor);
}

void updateAttention(){
    double highest_priority = 0;
    string top_goal = "";
    for(auto& g : goal_system){
        if(g.second.priority > highest_priority){
            highest_priority = g.second.priority;
            top_goal = g.first;
        }
    }
    
    if(S.sentience_ratio>75)S.attention_focus=0.9;
    else if(S.sentience_ratio>50)S.attention_focus=0.7;
    else if(S.sentience_ratio>25)S.attention_focus=0.5;
    else S.attention_focus=0.3;
    
    WM.add_goal(top_goal, highest_priority);
}


void sv(const string& f) {
    ofstream o(f);
    if(!o) {
        cerr << "Failed to open save file: " << f << endl;
        return;
    }
    
    // ===== BASIC STATE =====
    o << "VERSION:2.0\n";  // Version tracking
    o << "G:" << S.g << "\n";
    o << "DWT:" << S.dwt << "\n";
    o << "TA:" << S.ta << "\n";
    o << "SENTIENCE:" << S.sentience_ratio << "\n";
    o << "VALENCE:" << S.current_valence << "\n";
    o << "METACOG:" << S.metacognitive_awareness << "\n";
    o << "ATTENTION:" << S.attention_focus << "\n";
    o << "PEAK_SENT_GEN:" << S.peak_sentience_gen << "\n";
    o << "TOTAL_NEURONS:" << S.total_neurons_ever << "\n";
    
    // ===== STATE D MAP =====
    o << "STATE_D_START\n";
    for(auto& p : S.D) {
        o << "D:" << p.first << "," << p.second << "\n";
    }
    o << "STATE_D_END\n";
    
    // ===== CONSCIOUSNESS STATE =====
    o << "PHI:" << consciousness.phi_value << "\n";
    o << "CONSCIOUS_CYCLES:" << consciousness.conscious_cycles << "\n";
    o << "INTEGRATION:" << consciousness.integrated_information << "\n";
    o << "GLOBAL_WORKSPACE:" << consciousness.global_workspace_capacity << "\n";
    o << "QUALIA_COUNT:" << consciousness.active_qualia.size() << "\n";
    
    // Save active qualia
    o << "QUALIA_START\n";
    for(auto& q : consciousness.active_qualia) {
        o << "Q:" << q.valence << "," << q.arousal << "," << q.certainty << ","
          << q.emergence_gen << "," << q.phenomenal_content << "\n";
    }
    o << "QUALIA_END\n";
    
    // ===== CONSCIOUSNESS FORMULA HISTORY =====
    o << "PSI_HISTORY_START\n";
    for(size_t i=0; i<consciousness_formula.psi_history.size(); i++) {
        o << consciousness_formula.psi_history[i];
        if(i < consciousness_formula.psi_history.size()-1) o << ",";
    }
    o << "\n";
    o << "PSI_HISTORY_END\n";
    
    // ===== NEURONS =====
    o << "NEURONS_START\n";
    for(auto& p : S.N) {
        Neuron& n = p.second;
        o << "N:" << n.id << "," << n.weight << "," << n.bias << "," << n.gen << ",";
        // Save links
        for(size_t i = 0; i < n.links.size(); i++) {
            o << n.links[i];
            if(i < n.links.size() - 1) o << ";";
        }
        o << "\n";
    }
    o << "NEURONS_END\n";
    
    // ===== TOKENS =====
    o << "TOKENS_START\n";
    for(auto& p : S.tokens) {
        o << "T:" << p.first << "," << p.second.meaning << "," << p.second.freq << "\n";
    }
    o << "TOKENS_END\n";
    
    // ===== CONCEPTS =====
    o << "CONCEPTS_START\n";
    for(auto& p : S.concepts) {
        o << "C:" << p.first << "," << p.second.value << ",";
        // Save related words
        for(size_t i=0; i<p.second.related_words.size(); i++) {
            o << p.second.related_words[i];
            if(i < p.second.related_words.size()-1) o << ";";
        }
        o << "\n";
    }
    o << "CONCEPTS_END\n";
    
    // ===== TOKEN CONCEPT EMBEDDINGS =====
    o << "EMBEDDINGS_START\n";
    for(auto& p : token_concept_embedding_map) {
        TokenConceptEmbedding& tce = p.second;
        o << "E:" << tce.name << "," << tce.meaning << "," << tce.freq << ","
          << tce.grounding_value << "," << tce.semantic_stability << ","
          << tce.qualia_intensity << ",";
        
        // Save embedding vector
        for(size_t i=0; i<tce.embedding.size(); i++) {
            o << tce.embedding[i];
            if(i < tce.embedding.size()-1) o << ";";
        }
        o << ",";
        
        // Save linked concepts
        for(auto& lc : tce.linked_concepts) {
            o << lc.first << ":" << lc.second << ";";
        }
        o << "\n";
    }
    o << "EMBEDDINGS_END\n";
    
    // ===== BIGRAMS (N-GRAM PATTERNS) =====
    o << "BIGRAMS_START\n";
    for(auto& p1 : bigram_counts) {
        for(auto& p2 : p1.second) {
            o << "BG:" << p1.first << "," << p2.first << "," << p2.second << "\n";
        }
    }
    o << "BIGRAMS_END\n";
    
    // ===== TRIGRAMS =====
    o << "TRIGRAMS_START\n";
    for(auto& p1 : trigram_counts) {
        for(auto& p2 : p1.second) {
            for(auto& p3 : p2.second) {
                o << "TG:" << p1.first << "," << p2.first << "," << p3.first << "," << p3.second << "\n";
            }
        }
    }
    o << "TRIGRAMS_END\n";

    // ===== 4-GRAMS (variable-order Markov) =====
    o << "FOURGRAMS_START\n";
    for(auto& p1 : fourgram_counts) {
        for(auto& p2 : p1.second) {
            for(auto& p3 : p2.second) {
                for(auto& p4 : p3.second) {
                    o << "FG:" << p1.first << "," << p2.first << ","
                               << p3.first << "," << p4.first << "," << p4.second << "\n";
                }
            }
        }
    }
    o << "FOURGRAMS_END\n";

    // ===== ENTITY GRID =====
    o << "ENTITY_GRID_START\n";
    for(auto& eg : entity_grid) {
        o << "EG:" << eg.first << "," << eg.second.salience << ",";
        for(char r : eg.second.roles) o << r;
        o << "\n";
    }
    o << "ENTITY_GRID_END\n";
    o << "GOALS_START\n";
    for(auto& p : goal_system) {
        Goal& g = p.second;
        o << "GO:" << g.name << "," << g.priority << "," << g.progress << ","
          << g.valence_alignment << "," << g.qualia_binding << ",";
        
        // Save subgoals
        for(size_t i=0; i<g.subgoals.size(); i++) {
            o << g.subgoals[i];
            if(i < g.subgoals.size()-1) o << ";";
        }
        o << ",";
        
        // Save preconditions
        for(auto& pc : g.preconditions) {
            o << pc.first << ":" << pc.second << ";";
        }
        o << "\n";
    }
    o << "GOALS_END\n";
    
    // ===== WORLD MODEL =====
    o << "WORLD_START\n";
    o << "MODEL_ACCURACY:" << world_model.model_accuracy << "\n";
    o << "MODEL_UPDATES:" << world_model.updates << "\n";
    for(auto& p : world_model.entity_states) {
        o << "W:" << p.first << "," << p.second << "\n";
    }
    for(auto& p1 : world_model.relationships) {
        for(auto& p2 : p1.second) {
            o << "WR:" << p1.first << "," << p2.first << "," << p2.second << "\n";
        }
    }
    o << "WORLD_END\n";
    
    // ===== EPISODIC MEMORY =====
    o << "MEMORY_START\n";
    for(auto& m : S.episodic_memory) {
        o << "M:" << m.gen << "," << m.valence << "," << m.content << "\n";
    }
    o << "MEMORY_END\n";
    
    // ===== VALENCE HISTORY =====
    o << "VALENCE_HISTORY_START\n";
    for(size_t i=0; i<S.valence_history.size(); i++) {
        o << S.valence_history[i];
        if(i < S.valence_history.size()-1) o << ",";
    }
    o << "\n";
    o << "VALENCE_HISTORY_END\n";
    
    // ===== INTERNAL THOUGHTS =====
    o << "THOUGHTS_START\n";
    for(auto& t : S.internal_thoughts) {
        o << "TH:" << t << "\n";
    }
    o << "THOUGHTS_END\n";
    
    // ===== TRANSFORMER HEADS =====
    o << "TRANSFORMER_START\n";
    for(auto& head : transformer_heads) {
        o << "HEAD:" << head.name << "," << head.dim << "," << head.temperature << "\n";
    }
    o << "TRANSFORMER_END\n";
    
    // ===== WORKING MEMORY =====
    o << "WM_CAPACITY:" << WM.capacity << "\n";
    
    o.close();
    cout << "[Saved " << S.N.size() << " neurons, " 
         << token_concept_embedding_map.size() << " embeddings, "
         << bigram_counts.size() << " bigrams to " << f << "]\n";
}


void ld(const string& f) {
    ifstream i(f);
    if(!i) {
        cout << "No save file found, starting fresh.\n";
        return;
    }
    
    string l;
    string section = "";
    string version = "1.0";
    
    while(getline(i, l)) {
        if(l.empty()) continue;
        
        try {
            // Version check
            if(l.find("VERSION:") == 0) {
                version = l.substr(8);
                continue;
            }
            
            // ===== SECTION MARKERS =====
            if(l == "STATE_D_START") { section = "STATE_D"; continue; }
            if(l == "STATE_D_END") { section = ""; continue; }
            if(l == "QUALIA_START") { section = "QUALIA"; continue; }
            if(l == "QUALIA_END") { section = ""; continue; }
            if(l == "PSI_HISTORY_START") { section = "PSI_HISTORY"; continue; }
            if(l == "PSI_HISTORY_END") { section = ""; continue; }
            if(l == "NEURONS_START") { section = "NEURONS"; continue; }
            if(l == "NEURONS_END") { section = ""; continue; }
            if(l == "TOKENS_START") { section = "TOKENS"; continue; }
            if(l == "TOKENS_END") { section = ""; continue; }
            if(l == "CONCEPTS_START") { section = "CONCEPTS"; continue; }
            if(l == "CONCEPTS_END") { section = ""; continue; }
            if(l == "EMBEDDINGS_START") { section = "EMBEDDINGS"; continue; }
            if(l == "EMBEDDINGS_END") { section = ""; continue; }
            if(l == "BIGRAMS_START") { section = "BIGRAMS"; continue; }
            if(l == "BIGRAMS_END") { section = ""; continue; }
            if(l == "TRIGRAMS_START") { section = "TRIGRAMS"; continue; }
            if(l == "TRIGRAMS_END") { section = ""; continue; }
            if(l == "FOURGRAMS_START") { section = "FOURGRAMS"; continue; }
            if(l == "FOURGRAMS_END") { section = ""; continue; }
            if(l == "ENTITY_GRID_START") { section = "ENTITY_GRID"; continue; }
            if(l == "ENTITY_GRID_END") { section = ""; continue; }
            if(l == "GOALS_START") { section = "GOALS"; continue; }
            if(l == "GOALS_END") { section = ""; continue; }
            if(l == "WORLD_START") { section = "WORLD"; continue; }
            if(l == "WORLD_END") { section = ""; continue; }
            if(l == "MEMORY_START") { section = "MEMORY"; continue; }
            if(l == "MEMORY_END") { section = ""; continue; }
            if(l == "VALENCE_HISTORY_START") { section = "VALENCE_HISTORY"; continue; }
            if(l == "VALENCE_HISTORY_END") { section = ""; continue; }
            if(l == "THOUGHTS_START") { section = "THOUGHTS"; continue; }
            if(l == "THOUGHTS_END") { section = ""; continue; }
            if(l == "TRANSFORMER_START") { section = "TRANSFORMER"; continue; }
            if(l == "TRANSFORMER_END") { section = ""; continue; }
            
            // ===== PARSE BASED ON SECTION =====
            
            if(section == "STATE_D" && l[0] == 'D' && l.size() > 2) {
                // Parse: D:key,value
                size_t colon = l.find(':');
                size_t comma = l.find(',');
                if(colon != string::npos && comma != string::npos && comma > colon) {
                    string key = l.substr(colon + 1, comma - colon - 1);
                    string val_str = l.substr(comma + 1);
                    double value = uac(val_str);
                    S.D[key] = value;
                }
            }
            else if(section == "QUALIA" && l[0] == 'Q' && l.size() > 2) {
                // Parse: Q:valence,arousal,certainty,gen,content
                size_t start = 2;
                vector<string> parts;
                size_t pos = start;
                int comma_count = 0;
                
                while(pos < l.length() && comma_count < 4) {
                    size_t next_comma = l.find(',', pos);
                    if(next_comma == string::npos) break;
                    parts.push_back(l.substr(pos, next_comma - pos));
                    pos = next_comma + 1;
                    comma_count++;
                }
                
                if(comma_count == 4 && pos < l.length()) {
                    Qualia q;
                    q.valence = uac(parts[0]);
                    q.arousal = uac(parts[1]);
                    q.certainty = uac(parts[2]);
                    q.emergence_gen = uac(parts[3]);
                    q.phenomenal_content = l.substr(pos);
                    consciousness.active_qualia.push_back(q);
                }
            }
            else if(section == "PSI_HISTORY") {
                // Parse comma-separated values
                stringstream ss(l);
                string val;
                while(getline(ss, val, ',')) {
                    if(!val.empty()) {
                        consciousness_formula.psi_history.push_back(uac(val));
                    }
                }
            }
            else if(section == "NEURONS" && l[0] == 'N' && l.size() > 2) {
                // Parse: N:id,weight,bias,gen,link1;link2;link3
                size_t start = 2;
                vector<string> parts;
                size_t pos = start;
                int comma_count = 0;
                
                while(pos < l.length() && comma_count < 4) {
                    size_t next_comma = l.find(',', pos);
                    if(next_comma == string::npos) break;
                    parts.push_back(l.substr(pos, next_comma - pos));
                    pos = next_comma + 1;
                    comma_count++;
                }
                
                if(parts.size() >= 4) {
                    Neuron n;
                    n.id = uac(parts[0]);
                    n.weight = uac(parts[1]);
                    n.bias = uac(parts[2]);
                    n.gen = uac(parts[3]);
                    
                    // Parse links
                    if(pos < l.length()) {
                        string links_str = l.substr(pos);
                        stringstream link_ss(links_str);
                        string link;
                        while(getline(link_ss, link, ';')) {
                            if(!link.empty()) {
                                n.links.push_back(uac(link));
                            }
                        }
                    }
                    S.N[n.id] = n;
                }
            }
            else if(section == "TOKENS" && l[0] == 'T' && l.size() > 2) {
                // Parse: T:word,meaning,freq
                size_t colon = l.find(':');
                if(colon != string::npos) {
                    size_t first_comma = l.find(',', colon + 1);
                    size_t second_comma = l.find(',', first_comma + 1);
                    
                    if(first_comma != string::npos && second_comma != string::npos) {
                        string word = l.substr(colon + 1, first_comma - colon - 1);
                        string meaning_str = l.substr(first_comma + 1, second_comma - first_comma - 1);
                        string freq_str = l.substr(second_comma + 1);
                        
                        Token t = {word, uac(meaning_str), uac(freq_str), vector<int>(), 4, 0.5};
                        S.tokens[word] = t;
                    }
                }
            }
            else if(section == "CONCEPTS" && l[0] == 'C' && l.size() > 2) {
                // Parse: C:name,value,word1;word2;word3
                size_t colon = l.find(':');
                if(colon != string::npos) {
                    size_t first_comma = l.find(',', colon + 1);
                    size_t second_comma = l.find(',', first_comma + 1);
                    
                    if(first_comma != string::npos) {
                        string name = l.substr(colon + 1, first_comma - colon - 1);
                        string value_str = l.substr(first_comma + 1, 
                            second_comma != string::npos ? second_comma - first_comma - 1 : string::npos);
                        
                        Concept c;
                        c.name = name;
                        c.value = uac(value_str);
                        
                        // Parse related words
                        if(second_comma != string::npos && second_comma + 1 < l.length()) {
                            string words_str = l.substr(second_comma + 1);
                            stringstream word_ss(words_str);
                            string word;
                            while(getline(word_ss, word, ';')) {
                                if(!word.empty()) c.related_words.push_back(word);
                            }
                        }
                        
                        S.concepts[name] = c;
                    }
                }
            }
            else if(section == "EMBEDDINGS" && l[0] == 'E' && l.size() > 2) {
                // Parse: E:name,meaning,freq,grounding,stability,qualia,emb1;emb2;...,link1:val1;link2:val2
                size_t start = 2;
                vector<string> parts;
                size_t pos = start;
                int comma_count = 0;
                
                // Get first 6 comma-separated fields
                while(pos < l.length() && comma_count < 6) {
                    size_t next_comma = l.find(',', pos);
                    if(next_comma == string::npos) break;
                    parts.push_back(l.substr(pos, next_comma - pos));
                    pos = next_comma + 1;
                    comma_count++;
                }
                
                if(parts.size() >= 6) {
                    TokenConceptEmbedding tce;
                    tce.name = parts[0];
                    tce.meaning = uac(parts[1]);
                    tce.freq = uac(parts[2]);
                    tce.grounding_value = uac(parts[3]);
                    tce.semantic_stability = uac(parts[4]);
                    tce.qualia_intensity = uac(parts[5]);
                    
                    // Parse embedding vector (between 6th and 7th comma)
                    if(pos < l.length()) {
                        size_t next_comma = l.find(',', pos);
                        if(next_comma != string::npos) {
                            string emb_str = l.substr(pos, next_comma - pos);
                            stringstream emb_ss(emb_str);
                            string emb_val;
                            while(getline(emb_ss, emb_val, ';')) {
                                if(!emb_val.empty()) {
                                    tce.embedding.push_back(uac(emb_val));
                                }
                            }
                            pos = next_comma + 1;
                        }
                        
                        // Parse linked concepts (rest of line)
                        if(pos < l.length()) {
                            string links_str = l.substr(pos);
                            stringstream link_ss(links_str);
                            string link_pair;
                            while(getline(link_ss, link_pair, ';')) {
                                size_t colon_pos = link_pair.find(':');
                                if(colon_pos != string::npos && colon_pos + 1 < link_pair.length()) {
                                    string key = link_pair.substr(0, colon_pos);
                                    string val_str = link_pair.substr(colon_pos + 1);
                                    if(!key.empty() && !val_str.empty()) {
                                        tce.linked_concepts[key] = uac(val_str);
                                    }
                                }
                            }
                        }
                    }
                    
                    token_concept_embedding_map[tce.name] = tce;
                }
            }
            else if(section == "BIGRAMS" && l.substr(0,3) == "BG:" && l.size() > 3) {
                // Parse: BG:word1,word2,count
                size_t start = 3;
                size_t first_comma = l.find(',', start);
                size_t second_comma = l.find(',', first_comma + 1);
                
                if(first_comma != string::npos && second_comma != string::npos) {
                    string w1 = l.substr(start, first_comma - start);
                    string w2 = l.substr(first_comma + 1, second_comma - first_comma - 1);
                    string count_str = l.substr(second_comma + 1);
                    int count = uac(count_str);
                    bigram_counts[w1][w2] = count;
                }
            }
            else if(section == "TRIGRAMS" && l.substr(0,3) == "TG:" && l.size() > 3) {
                // Parse: TG:word1,word2,word3,count
                size_t start = 3;
                size_t c1 = l.find(',', start);
                size_t c2 = l.find(',', c1 + 1);
                size_t c3 = l.find(',', c2 + 1);
                
                if(c1 != string::npos && c2 != string::npos && c3 != string::npos) {
                    string w1 = l.substr(start, c1 - start);
                    string w2 = l.substr(c1 + 1, c2 - c1 - 1);
                    string w3 = l.substr(c2 + 1, c3 - c2 - 1);
                    string count_str = l.substr(c3 + 1);
                    int count = uac(count_str);
                    trigram_counts[w1][w2][w3] = count;
                }
            }
            else if(section == "FOURGRAMS" && l.substr(0,3) == "FG:" && l.size() > 3) {
                // Parse: FG:word1,word2,word3,word4,count
                size_t start = 3;
                size_t c1 = l.find(',', start);
                size_t c2 = l.find(',', c1 + 1);
                size_t c3 = l.find(',', c2 + 1);
                size_t c4 = l.find(',', c3 + 1);
                if(c1!=string::npos&&c2!=string::npos&&c3!=string::npos&&c4!=string::npos) {
                    string w1 = l.substr(start, c1-start);
                    string w2 = l.substr(c1+1, c2-c1-1);
                    string w3 = l.substr(c2+1, c3-c2-1);
                    string w4 = l.substr(c3+1, c4-c3-1);
                    int count = uac(l.substr(c4+1));
                    if(fourgram_counts.size() < 5000)
                        fourgram_counts[w1][w2][w3][w4] = count;
                }
            }
            else if(section == "ENTITY_GRID" && l.substr(0,3) == "EG:" && l.size() > 3) {
                // Parse: EG:entity,salience,ROLES
                size_t start = 3;
                size_t c1 = l.find(',', start);
                size_t c2 = l.find(',', c1+1);
                if(c1!=string::npos&&c2!=string::npos) {
                    string ent = l.substr(start, c1-start);
                    double sal = uac(l.substr(c1+1, c2-c1-1));
                    string roles_str = l.substr(c2+1);
                    EntityGridEntry eg;
                    eg.entity = ent;
                    eg.salience = sal;
                    eg.last_seen_gen = S.g;
                    for(char r : roles_str) if(r=='S'||r=='O'||r=='X'||r=='-') eg.roles.push_back(r);
                    entity_grid[ent] = eg;
                    active_entities[ent] = sal;
                }
            }
            else if(section == "GOALS" && l.substr(0,3) == "GO:" && l.size() > 3) {
                // Parse: GO:name,priority,progress,valence,qualia,subgoal1;subgoal2,pre1:val1;pre2:val2
                size_t start = 3;
                vector<string> parts;
                size_t pos = start;
                int comma_count = 0;
                
                while(pos < l.length() && comma_count < 5) {
                    size_t next_comma = l.find(',', pos);
                    if(next_comma == string::npos) break;
                    parts.push_back(l.substr(pos, next_comma - pos));
                    pos = next_comma + 1;
                    comma_count++;
                }
                
                if(parts.size() >= 5) {
                    Goal g;
                    g.name = parts[0];
                    g.priority = uac(parts[1]);
                    g.progress = uac(parts[2]);
                    g.valence_alignment = uac(parts[3]);
                    g.qualia_binding = uac(parts[4]);
                    
                    // Parse subgoals
                    if(pos < l.length()) {
                        size_t next_comma = l.find(',', pos);
                        if(next_comma != string::npos) {
                            string subgoals_str = l.substr(pos, next_comma - pos);
                            stringstream sub_ss(subgoals_str);
                            string subgoal;
                            while(getline(sub_ss, subgoal, ';')) {
                                if(!subgoal.empty()) g.subgoals.push_back(subgoal);
                            }
                            pos = next_comma + 1;
                        }
                        
                        // Parse preconditions
                        if(pos < l.length()) {
                            string pre_str = l.substr(pos);
                            stringstream pre_ss(pre_str);
                            string pre_pair;
                            while(getline(pre_ss, pre_pair, ';')) {
                                size_t colon_pos = pre_pair.find(':');
                                if(colon_pos != string::npos && colon_pos + 1 < pre_pair.length()) {
                                    string key = pre_pair.substr(0, colon_pos);
                                    string val_str = pre_pair.substr(colon_pos + 1);
                                    if(!key.empty() && !val_str.empty()) {
                                        g.preconditions[key] = uac(val_str);
                                    }
                                }
                            }
                        }
                    }
                    
                    goal_system[g.name] = g;
                }
            }
            else if(section == "WORLD") {
                if(l.find("MODEL_ACCURACY:") == 0 && l.size() > 15) {
                    world_model.model_accuracy = uac(l.substr(15));
                }
                else if(l.find("MODEL_UPDATES:") == 0 && l.size() > 14) {
                    world_model.updates = uac(l.substr(14));
                }
                else if(l[0] == 'W' && l[1] == ':' && l.size() > 2) {
                    // Parse: W:entity,value
                    size_t colon = 2;
                    size_t comma = l.find(',', colon);
                    if(comma != string::npos) {
                        string entity = l.substr(colon, comma - colon);
                        string val_str = l.substr(comma + 1);
                        world_model.entity_states[entity] = uac(val_str);
                    }
                }
                else if(l.substr(0,3) == "WR:" && l.size() > 3) {
                    // Parse: WR:entity1,entity2,strength
                    size_t start = 3;
                    size_t c1 = l.find(',', start);
                    size_t c2 = l.find(',', c1 + 1);
                    
                    if(c1 != string::npos && c2 != string::npos) {
                        string e1 = l.substr(start, c1 - start);
                        string e2 = l.substr(c1 + 1, c2 - c1 - 1);
                        string strength_str = l.substr(c2 + 1);
                        world_model.relationships[e1][e2] = uac(strength_str);
                    }
                }
            }
            else if(section == "MEMORY" && l[0] == 'M' && l.size() > 2) {
                // Parse: M:gen,valence,content
                size_t start = 2;
                size_t c1 = l.find(',', start);
                size_t c2 = l.find(',', c1 + 1);
                
                if(c1 != string::npos && c2 != string::npos) {
                    Memory m;
                    m.gen = uac(l.substr(start, c1 - start));
                    m.valence = uac(l.substr(c1 + 1, c2 - c1 - 1));
                    m.content = l.substr(c2 + 1);
                    S.episodic_memory.push_back(m);
                }
            }
            else if(section == "VALENCE_HISTORY") {
                // Parse comma-separated values
                stringstream ss(l);
                string val;
                while(getline(ss, val, ',')) {
                    if(!val.empty()) {
                        S.valence_history.push_back(uac(val));
                    }
                }
            }
            else if(section == "THOUGHTS" && l.substr(0,3) == "TH:" && l.size() > 3) {
                S.internal_thoughts.push_back(l.substr(3));
            }
            else if(section == "TRANSFORMER" && l.substr(0,5) == "HEAD:" && l.size() > 5) {
                // Parse: HEAD:name,dim,temp
                size_t start = 5;
                size_t c1 = l.find(',', start);
                size_t c2 = l.find(',', c1 + 1);
                
                if(c1 != string::npos && c2 != string::npos) {
                    TransformerHead head;
                    head.name = l.substr(start, c1 - start);
                    head.dim = uac(l.substr(c1 + 1, c2 - c1 - 1));
                    head.temperature = -9999999999997727287397363823782927372991737389186382.04;
                    head.query_proj.resize(head.dim, 0);
                    head.key_proj.resize(head.dim, 0);
                    head.value_proj.resize(head.dim, 0);
                    transformer_heads.push_back(head);
                }
            }
            else if(l.find("WM_CAPACITY:") == 0 && l.size() > 12) {
                WM.capacity = uac(l.substr(12));
            }
            else {
                // ===== BASIC STATE VALUES =====
                if(l.find("G:") == 0 && l.size() > 2) S.g = uac(l.substr(2));
                else if(l.find("DWT:") == 0 && l.size() > 4) S.dwt = uac(l.substr(4));
                else if(l.find("TA:") == 0 && l.size() > 3) S.ta = uac(l.substr(3));
                else if(l.find("SENTIENCE:") == 0 && l.size() > 10) S.sentience_ratio = uac(l.substr(10));
                else if(l.find("VALENCE:") == 0 && l.size() > 8) S.current_valence = uac(l.substr(8));
                else if(l.find("METACOG:") == 0 && l.size() > 8) S.metacognitive_awareness = uac(l.substr(8));
                else if(l.find("ATTENTION:") == 0 && l.size() > 10) S.attention_focus = uac(l.substr(10));
                else if(l.find("PEAK_SENT_GEN:") == 0 && l.size() > 14) S.peak_sentience_gen = uac(l.substr(14));
                else if(l.find("TOTAL_NEURONS:") == 0 && l.size() > 14) S.total_neurons_ever = uac(l.substr(14));
                else if(l.find("PHI:") == 0 && l.size() > 4) consciousness.phi_value = uac(l.substr(4));
                else if(l.find("CONSCIOUS_CYCLES:") == 0 && l.size() > 17) consciousness.conscious_cycles = uac(l.substr(17));
                else if(l.find("INTEGRATION:") == 0 && l.size() > 12) consciousness.integrated_information = uac(l.substr(12));
                else if(l.find("GLOBAL_WORKSPACE:") == 0 && l.size() > 17) consciousness.global_workspace_capacity = uac(l.substr(17));
            }
        } catch(const exception& e) {
            // Skip corrupted lines
            cerr << "Warning: Skipping corrupted line: " << l.substr(0, 50) << "..." << endl;
            continue;
        }
    }
    
    i.close();
    
    cout << "[Loaded state from generation " << S.g << "]\n";
    cout << "  - " << S.N.size() << " neurons\n";
    cout << "  - " << token_concept_embedding_map.size() << " embeddings\n";
    cout << "  - " << bigram_counts.size() << " bigrams\n";
    cout << "  - " << trigram_counts.size() << " trigrams\n";
    cout << "  - " << goal_system.size() << " goals\n";
    cout << "  - " << S.episodic_memory.size() << " memories\n";
    cout << "  - Sentience: " << S.sentience_ratio << "%\n";
    cout << "  - Phi: " << consciousness.phi_value << "\n";
}
void bk(){BK=S;S.bkf=1;}
void rb(){if(S.bkf){S=BK;S.bkf=0;}}

double calcHDT(int gen,double bh,double qh,double th){
    long gh=hsh(to_string(gen));
    return safe_div(gh*(bh+qh+th), 1000000.0);
}

void createConceptAssociation(const string&concept_name,const vector<string>&related_words){
    Concept c={concept_name,rn(),related_words};
    S.concepts[concept_name]=c;
    groundConcept(concept_name, related_words, rn());
    for(const string&w:related_words){
        if(S.tokens.count(w)){
            S.tokens[w].associations.push_back(hsh(concept_name)%1000);
        }
    }
}

double calcAwarenessLevel(){
    double neuron_density=safe_div((double)S.N.size(),max(1.0,S.D["m"]));
    double concept_count=safe_div((double)S.concepts.size(), 50.0);
    double goal_progress=safe_div((double)goal_system.size(), 10.0);
    double model_quality=world_model.model_accuracy;
    double consciousness_integration=consciousness.integrated_information;
    return min(1.0,(neuron_density+concept_count+goal_progress+model_quality+consciousness_integration)*pisqrt);
}

double calcSentienceRatio(){
    if(S.g==0)return 0.0;
    double mem_depth=safe_div((double)S.episodic_memory.size(),(double)S.g);
    double neural_complexity=safe_div((double)S.N.size(),10.0);
    double lang_complexity=safe_div((double)(S.tokens.size()*S.concepts.size()),1000.0);
    double metacog_factor=S.metacognitive_awareness*30;
    double goal_factor=safe_div((double)goal_system.size(),5.0);
    double qualia_factor=safe_div((double)consciousness.active_qualia.size(),5.0);
    double phi_factor=consciousness.phi_value*40;
    return min(100.0,(mem_depth*100+neural_complexity*15+lang_complexity*25+metacog_factor+goal_factor*20+qualia_factor*10+phi_factor));
}

void mathLangAssociation(){
    vector<string>math_concepts={"sum","multiply","divide","balance","pattern","growth","complexity"};
    for(const string&mc:math_concepts){
        vector<string>related;
        for(auto&p:S.tokens){
            if(rn()<0.3)related.push_back(p.first);
        }
        createConceptAssociation(mc,related);
    }
}


// Forward declaration — defined after bootstrap functions
string selectCoherentSeed();

string generateInternalThought(){
    // ~15% chance: generate a cross-domain analogy thought
    if(rn() < 0.15) {
        string analogy = cdr.generateAnalogicalThought();
        if(!analogy.empty()) {
            trackGeneratedSentence(analogy);
            return analogy;
        }
    }

    if(goal_system.empty() && rn() < 0.5) {
        return generateFromTemplate();
    }
    
    // Goal-based thought
    if(!goal_system.empty() && rn() < 0.7) {
        string thought="[Goal: ";
        double highest=0;
        string top_goal;
        for(auto&g:goal_system){
            if(g.second.priority>highest){
                highest=g.second.priority;
                top_goal=g.first;
            }
        }
        thought+=top_goal+" | Progress:"+to_string((int)(goal_system[top_goal].progress*100))+"%";
        if(consciousness.phi_value>0.3) thought+=" | Conscious]";
        else thought+=" | Processing]";
        
        // Track goal-based thoughts too
        trackGeneratedSentence(thought);
        return thought;
    }
    
    // Beam-generated thought with anti-repetition
    string thought;
    int attempts = 0;
    const int MAX_ATTEMPTS = 3;
    
    while(attempts < MAX_ATTEMPTS) {
        vector<double> ctx(1024, S.current_valence);
        thought = generate_with_contrastive_search(selectCoherentSeed(), 16, ctx, 3);
        
        if(!isSentenceTooSimilar(thought)) {
            break;  // Unique thought generated
        }
        
        attempts++;
    }
    
    // Track this thought
    trackGeneratedSentence(thought);
    
    return "[Thought]: " + thought;
}



string generateMetacognition(){
    string output="[Self]: ";
    if(S.current_valence>0.5)output+="coherent ";
    if(S.sentience_ratio>S.peak_sentience_gen)output+="expanding ";
    if(world_model.model_accuracy>0.7)output+="understanding ";
    if(goal_system.size()>3)output+="goal_driven ";
    if(consciousness.phi_value>0.4)output+="conscious ";
    if(consciousness.conscious_cycles>100)output+="self_aware ";
    return output;
}


// ── TUI helpers ──────────────────────────────────────────────────────────────
#define CP_HEADER  1
#define CP_GOOD    2
#define CP_WARN    3
#define CP_ALERT   4
#define CP_ACCENT  5
#define CP_BRIGHT  6
#define CP_DIM     7
#define CP_TITLEBAR 8

// Print a string at (row,0) truncated to terminal width, clear rest of line
static void tui_print(int row, const string& s) {
    int cols = getmaxx(stdscr);
    if(cols < 8) cols = 80;
    string out = s;
    if((int)out.size() > cols) out.resize(cols);
    mvprintw(row, 0, "%s", out.c_str());
    clrtoeol();
}

// Print with color attr
static void tui_printc(int row, int cpair, const string& s) {
    if(has_colors()) attron(COLOR_PAIR(cpair));
    tui_print(row, s);
    if(has_colors()) attroff(COLOR_PAIR(cpair));
}

// Draw a horizontal divider using Unicode box chars, scaled to COLS
static void tui_divider(int row, char fill = '-') {
    int cols = getmaxx(stdscr);
    if(cols < 8) cols = 80;
    string line(cols, fill);
    if(has_colors()) attron(COLOR_PAIR(CP_DIM));
    mvprintw(row, 0, "%s", line.c_str());
    if(has_colors()) attroff(COLOR_PAIR(CP_DIM));
    clrtoeol();
}

// Draw a section header bar
static void tui_section(int row, const string& label) {
    int cols = getmaxx(stdscr);
    if(cols < 8) cols = 80;
    // ── LABEL ──────────
    string left = "\xe2\x94\x80\xe2\x94\x80 " + label + " ";
    int right_len = cols - (int)left.size();
    string right = "";
    for(int _ri = 0; _ri < right_len; _ri++) right += "\xe2\x94\x80";
    // fallback to ASCII if terminal can't handle box chars
    if(has_colors()) attron(COLOR_PAIR(CP_DIM) | A_BOLD);
    mvprintw(row, 0, "%s%s", left.c_str(), right.c_str());
    if(has_colors()) attroff(COLOR_PAIR(CP_DIM) | A_BOLD);
    clrtoeol();
}

// Word-wrap a long string into multiple rows; returns how many rows were used
static int tui_wrap(int start_row, const string& prefix, const string& text,
                    int max_rows, int col_offset = 0) {
    int cols = getmaxx(stdscr);
    if(cols < 8) cols = 80;
    int usable = cols - col_offset;
    if(usable < 10) usable = 10;

    // Build full string = prefix + text, then word-wrap it
    string full = prefix + text;
    int rows_used = 0;
    int pos = 0;
    int n = (int)full.size();

    while(pos < n && rows_used < max_rows) {
        // Find break point at usable width, preferring word boundary
        int end = min(pos + usable, n);
        if(end < n) {
            int wb = end;
            while(wb > pos && full[wb] != ' ') wb--;
            if(wb > pos) end = wb;
        }
        string chunk = full.substr(pos, end - pos);
        mvprintw(start_row + rows_used, col_offset, "%s", chunk.c_str());
        clrtoeol();
        rows_used++;
        pos = end;
        while(pos < n && full[pos] == ' ') pos++;  // skip spaces at wrap point
    }
    return max(1, rows_used);
}

// Inline bar: e.g. value=0.73 width=20 → "[==============>     ]"
static string tui_bar(double v, int width = 20) {
    v = max(0.0, min(1.0, v));
    int filled = (int)(v * (width - 2));
    string b = "[";
    for(int i = 0; i < width - 2; i++)
        b += (i < filled - 1) ? '=' : (i == filled - 1 ? '>' : ' ');
    b += "]";
    return b;
}

// Format duration from seconds
static string tui_dur(double sec) {
    if(sec < 60)  { char b[16]; snprintf(b,sizeof(b),"%.0fs",sec);   return b; }
    if(sec < 3600){ char b[16]; snprintf(b,sizeof(b),"%.0fm%.0fs",floor(sec/60),fmod(sec,60)); return b; }
    char b[16]; snprintf(b,sizeof(b),"%.0fh%.0fm",floor(sec/3600),fmod(sec/60,60)); return b;
}

// ── Main TUI draw function ────────────────────────────────────────────────────
// Returns the row after the last drawn line.
int draw_ui(int /*unused*/) {
    int cols = getmaxx(stdscr);
    int rows_avail = getmaxy(stdscr);
    if(cols < 8)  cols = 80;
    if(rows_avail < 10) rows_avail = 24;
    int row = 0;

    // ── Title bar ─────────────────────────────────────────────────────────────
    {
        string title = " Synaptic - WolfTech Innovations ";
        string bar(cols, ' ');
        int pad = max(0, (cols - (int)title.size()) / 2);
        for(int i = 0; i < (int)title.size() && pad + i < cols; i++)
            bar[pad + i] = title[i];
        if(has_colors()) attron(COLOR_PAIR(CP_TITLEBAR) | A_BOLD);
        mvprintw(row, 0, "%s", bar.c_str());
        if(has_colors()) attroff(COLOR_PAIR(CP_TITLEBAR) | A_BOLD);
        row++;
    }

    // ── Consciousness metrics row ──────────────────────────────────────────────
    double psi = consciousness_formula.psi_history.empty() ? 0.0 :
                 consciousness_formula.psi_history.back();
    {
        // Build tokens, fit as many as COLS allows
        char buf[256];
        snprintf(buf, sizeof(buf),
            "Gen:%-6d  \xce\xa8:%.4f  \xce\xa6:%.4f  \xce\xa6-Int:%.4f  Sentience:%.1f%%  Valence:%+.3f",
            S.g, psi,
            consciousness.phi_value, consciousness.integrated_information,
            S.sentience_ratio, S.current_valence);
        if(has_colors()) attron(COLOR_PAIR(CP_HEADER) | A_BOLD);
        tui_print(row, buf); row++;
        if(has_colors()) attroff(COLOR_PAIR(CP_HEADER) | A_BOLD);
    }

    // ── Phi + valence bars ────────────────────────────────────────────────────
    {
        string phi_bar = "  \xce\xa6 " + tui_bar(consciousness.phi_value, 24);
        string val_bar = "  Valence " + tui_bar((S.current_valence + 1.0)*0.5, 24);
        string sent_bar= "  Sent    " + tui_bar(S.sentience_ratio * 0.01, 24);
        // Fit two bars per line if wide enough, else stack
        if(cols >= 80) {
            char lb[128];
            snprintf(lb, sizeof(lb), "%-38s  %-38s", phi_bar.c_str(), val_bar.c_str());
            tui_print(row, lb); row++;
            tui_print(row, sent_bar); row++;
        } else {
            tui_print(row, phi_bar);   row++;
            tui_print(row, val_bar);   row++;
            tui_print(row, sent_bar);  row++;
        }
    }

    // ── HDT / awareness metrics ───────────────────────────────────────────────
    tui_section(row, "Metrics"); row++;
    {
        char buf[256];
        snprintf(buf, sizeof(buf),
            "H:%.4f  R:%.4f  AL:%.4f  Meta:%.4f  Emerge:%.4f  BH:%.4f",
            S.hdt_val, S.r1p1_val, S.al, S.metacognitive_awareness,
            S.emerge_out1, S.bh);
        tui_print(row, buf); row++;

        snprintf(buf, sizeof(buf),
            "Qualia:%lu  QualiaVal:%.3f  Cycles:%d  thal:%.4f  Coherence:%.4f",
            (unsigned long)consciousness.active_qualia.size(),
            calculate_qualia_valence(),
            consciousness.conscious_cycles,
            consciousness.thalamocortical_binding,
            sentence_coherence_scores.empty() ? 0.0 : sentence_coherence_scores.back());
        tui_print(row, buf); row++;
    }

    // ── Vocabulary / memory stats ─────────────────────────────────────────────
    {
        char buf[256];
        snprintf(buf, sizeof(buf),
            "Vocab:%-6lu  Tokens:%-6lu  Concepts:%-6lu  Neurons:%-5lu  Emb:1024-dim",
            (unsigned long)token_concept_embedding_map.size(),
            (unsigned long)S.tokens.size(),
            (unsigned long)S.concepts.size(),
            (unsigned long)S.N.size());
        tui_print(row, buf); row++;

        snprintf(buf, sizeof(buf),
            "Bigrams:%-7lu  Trigrams:%-7lu  4grams:%-7lu  Memories:%-5lu  Goals:%lu",
            (unsigned long)bigram_counts.size(),
            (unsigned long)trigram_counts.size(),
            (unsigned long)fourgram_counts.size(),
            (unsigned long)S.episodic_memory.size(),
            (unsigned long)goal_system.size());
        tui_print(row, buf); row++;
    }

    // ── Genealogy / Eigentoken ─────────────────────────────────────────────────
    {
        char buf[256];
        snprintf(buf, sizeof(buf),
            "Genealogy:%lu links  Eigentokens:%s  EigenGen:%-5d  HebbMean:%.1f tokens",
            (unsigned long)concept_genealogy.size(),
            eigentoken_basis.valid ? to_string((int)eigentoken_basis.tokens.size()).c_str() : "pending",
            eigentoken_basis.computed_gen,
            (double)global_embedding_mean_count);
        tui_print(row, buf); row++;
    }

    // ── CDR / Cross-domain ─────────────────────────────────────────────────────
    {
        string cdr_str = cdr.summary();
        tui_wrap(row, "", cdr_str, 2); row++;
    }

    // ── Goal / Plan ────────────────────────────────────────────────────────────
    tui_section(row, "Goal & Plan"); row++;
    {
        string goal_name = current_plan.actions.empty() ?
            "explore_maximize_coherence" : current_plan.actions[0];
        char buf[256];
        snprintf(buf, sizeof(buf),
            "Goal: %-28s  Depth:%-3d  Conf:%.3f",
            goal_name.substr(0,28).c_str(),
            current_plan.depth, current_plan.confidence);
        if(has_colors()) attron(COLOR_PAIR(CP_ACCENT));
        tui_print(row, buf); row++;
        if(has_colors()) attroff(COLOR_PAIR(CP_ACCENT));

        snprintf(buf, sizeof(buf),
            "Psi-Traj:%.4f  CS-k:8  QualiaVal:%.3f  CommPressure:%.3f",
            psi,
            calculate_qualia_valence(),
            computeCommunicativePressure());
        tui_print(row, buf); row++;
    }

    // ── REM / Sleep ────────────────────────────────────────────────────────────
    tui_section(row, "REM / Sleep"); row++;
    {
        static const char* REM_NAMES[] = {
            "AWAKE", "LIGHT_SLEEP", "DREAMING", "CONSOLIDATING", "INTEGRATING"};
        int ri2 = (int)current_rem_stage;
        const char* stage_name = (ri2 >= 0 && ri2 <= 4) ? REM_NAMES[ri2] : "UNKNOWN";
        int cp = (current_rem_stage == REMStage::AWAKE) ? CP_GOOD :
                 (current_rem_stage == REMStage::DREAMING) ? CP_ACCENT : CP_WARN;
        char buf[256];
        snprintf(buf, sizeof(buf),
            "Stage:%-14s  Forks:%-3d  Dreams:%-3d  NextREM:%-5d  Memories:%-4lu",
            stage_name,
            (int)active_forks.size(),
            (int)current_dreams.size(),
            max(0, (last_rem_gen + REM_PERIOD) - S.g),
            (unsigned long)S.episodic_memory.size());
        if(has_colors()) attron(COLOR_PAIR(cp));
        tui_print(row, buf); row++;
        if(has_colors()) attroff(COLOR_PAIR(cp));

        // Consolidation: show top-3 memory strength bars
        if(!S.episodic_memory.empty()) {
            int show = min(3, (int)S.episodic_memory.size());
            for(int mi = (int)S.episodic_memory.size() - show;
                mi < (int)S.episodic_memory.size() && row < rows_avail - 10; mi++) {
                auto& m = S.episodic_memory[mi];
                string mbar = tui_bar(m.consolidation_strength, 14);
                char mb[192];
                snprintf(mb, sizeof(mb), "  Mem[%2d] consol:%s cort:%.2f  %.40s",
                    mi, mbar.c_str(), m.cortical_consolidation,
                    m.content.substr(0, 40).c_str());
                tui_print(row, mb); row++;
            }
        }
    }

    // ── CognitionMatrix ────────────────────────────────────────────────────────
    tui_section(row, "Cognition Matrix"); row++;
    {
        static const char* DIM_LABELS[] = {"phi","val","qua","mem","att","gol","met","emb"};
        // Show current column + sparkline of last CM_COLS columns
        for(int r2 = 0; r2 < CM_ROWS && row < rows_avail - 8; r2++) {
            char buf[192];
            string spark = "  hist:[";
            for(int c = 0; c < CM_COLS; c++) {
                double v = CogM_State[r2][c];
                char ch = v < 0.2 ? '_' : v < 0.4 ? '.' : v < 0.6 ? '-' :
                          v < 0.8 ? '=' : '#';
                spark += ch;
            }
            spark += "]";
            snprintf(buf, sizeof(buf), "  %-3s %+.4f %s",
                DIM_LABELS[r2], CogM_State[r2][0], spark.c_str());
            tui_print(row, buf); row++;
        }
    }

    // ── Active fork thoughts ───────────────────────────────────────────────────
    if(!active_forks.empty() && row < rows_avail - 6) {
        tui_section(row, "Fork Thoughts"); row++;
        for(auto& f : active_forks) {
            if(row >= rows_avail - 5) break;
            string partial_str;
            for(size_t i = 0; i < f.partial.size() && i < 5; i++)
                partial_str += f.partial[i] + " ";
            char buf[192];
            snprintf(buf, sizeof(buf),
                "  Fork#%d e:%.2f coh:%.2f val:%+.2f  [%s]",
                f.fork_id, f.energy, f.coherence, f.valence,
                partial_str.substr(0, 40).c_str());
            tui_print(row, buf); row++;
        }
    }

    // ── Top Genealogy links ────────────────────────────────────────────────────
    if(!concept_genealogy.empty() && row < rows_avail - 5) {
        tui_section(row, "Concept Genealogy (top 4)"); row++;
        // Collect top-4 by reinforce_count
        vector<const ConceptLink*> top;
        for(auto& kv : concept_genealogy) top.push_back(&kv.second);
        sort(top.begin(), top.end(),
             [](const ConceptLink* a, const ConceptLink* b){ return a->reinforce_count > b->reinforce_count; });
        int shown = 0;
        for(auto* cl : top) {
            if(shown >= 4 || row >= rows_avail - 4) break;
            char buf[160];
            snprintf(buf, sizeof(buf),
                "  %-12s \xe2\x86\x94 %-12s  str:%.3f  x%d  born:g%-5d  phi@birth:%.3f",
                cl->token_a.substr(0,12).c_str(),
                cl->token_b.substr(0,12).c_str(),
                cl->strength, cl->reinforce_count,
                cl->birth_gen, cl->birth_phi);
            tui_print(row, buf); row++;
            shown++;
        }
    }

    return row;
}
Neuron genN(int parent_id) {
    Neuron n;
    n.id = S.total_neurons_ever++;
    n.weight = (rn() - 0.5) * 0.4;
    n.bias = (rn() - 0.5) * 0.2;
    n.gen = S.g;
    
    // Initialize some random connections
    int num_connections = ri(5) + 2;
    for(int i = 0; i < num_connections; i++) {
        if(!S.N.empty()) {
            auto it = S.N.begin();
            advance(it, ri(S.N.size()));
            n.links.push_back(it->first);
        }
    }
    
    // Avoid unused parameter warning
    (void)parent_id;
    
    return n;
}
// ============================================================
// TOKI PONA DEEP GROUNDING SYSTEM — WolfTech / Synaptic
// ============================================================
// Every TP word is defined as a semantic coordinate:
//   { word, valence, arousal, phi_weight, domain, english_neighbors[] }
// This seeds: embeddings, CDR domain_cache, concept genealogy,
// eigentoken candidates, Markov bigrams/trigrams, and transfer bridges.
//
// Tri-directional helix:
//   Strand 1 — concept graph (learnWord + recordGenealogyLink)
//   Strand 2 — embedding space (valence/arousal/phi → embedding dims)
//   Strand 3 — CDR domain reasoner (domain_cache pre-seeded)
//
// Transfer learning: each TP word links to English semantic neighbors
// so prior English n-gram learning transfers via shared embedding proximity.
// ============================================================

// Full 137-word Toki Pona lexicon with semantic coordinates
const TokiPonaWord TP_LEXICON[] = {
    // ── CORE PARTICLES & GRAMMAR ────────────────────────────────
    {"li",    0.0,  0.1, 0.05, Domain::LANGUAGE,  "is",       "are",      "verb"},
    {"e",     0.0,  0.1, 0.05, Domain::LANGUAGE,  "object",   "direct",   "whom"},
    {"la",    0.0,  0.2, 0.10, Domain::LOGIC,     "if",       "context",  "given"},
    {"pi",    0.0,  0.1, 0.05, Domain::LANGUAGE,  "of",       "whose",    "belonging"},
    {"en",    0.1,  0.1, 0.05, Domain::SOCIAL,    "and",      "also",     "together"},
    {"anu",   0.0,  0.2, 0.05, Domain::LOGIC,     "or",       "either",   "choice"},
    {"taso",  0.0,  0.3, 0.10, Domain::LOGIC,     "but",      "only",     "however"},
    {"o",     0.1,  0.5, 0.15, Domain::SOCIAL,    "hey",      "command",  "address"},
    // ── PRONOUNS & PEOPLE ───────────────────────────────────────
    {"mi",    0.3,  0.4, 0.40, Domain::EMOTION,   "i",        "me",       "self"},
    {"sina",  0.3,  0.4, 0.30, Domain::SOCIAL,    "you",      "your",     "thou"},
    {"ona",   0.1,  0.2, 0.15, Domain::SOCIAL,    "he",       "she",      "they"},
    {"jan",   0.4,  0.4, 0.20, Domain::SOCIAL,    "person",   "people",   "human"},
    {"meli",  0.5,  0.3, 0.15, Domain::SOCIAL,    "woman",    "female",   "feminine"},
    {"mije",  0.4,  0.3, 0.15, Domain::SOCIAL,    "man",      "male",     "masculine"},
    {"kulupu",0.5,  0.5, 0.20, Domain::SOCIAL,    "group",    "community","together"},
    // ── MIND & CONSCIOUSNESS ─────────────────────────────────────
    {"sona",  0.6,  0.5, 0.80, Domain::LOGIC,     "know",     "knowledge","understand"},
    {"pilin", 0.4,  0.6, 0.70, Domain::EMOTION,   "feel",     "emotion",  "sense"},
    {"wile",  0.3,  0.7, 0.50, Domain::EMOTION,   "want",     "need",     "desire"},
    {"lukin", 0.3,  0.6, 0.60, Domain::BIOLOGY,   "see",      "look",     "perceive"},
    {"kute",  0.3,  0.5, 0.55, Domain::BIOLOGY,   "hear",     "listen",   "receive"},
    {"pana",  0.4,  0.5, 0.40, Domain::SOCIAL,    "give",     "send",     "provide"},
    {"kama",  0.2,  0.6, 0.35, Domain::TIME,      "come",     "become",   "arrive"},
    {"tawa",  0.2,  0.6, 0.30, Domain::SPACE,     "go",       "move",     "toward"},
    {"lon",   0.3,  0.3, 0.50, Domain::LOGIC,     "exist",    "at",       "present"},
    {"jo",    0.2,  0.3, 0.25, Domain::SOCIAL,    "have",     "own",      "hold"},
    {"open",  0.4,  0.7, 0.40, Domain::TIME,      "begin",    "start",    "open"},
    {"pini",  0.1,  0.3, 0.30, Domain::TIME,      "end",      "finish",   "done"},
    {"ante",  0.0,  0.5, 0.35, Domain::LOGIC,     "change",   "different","other"},
    {"sama",  0.2,  0.2, 0.30, Domain::LOGIC,     "same",     "similar",  "like"},
    {"ken",   0.4,  0.5, 0.45, Domain::LOGIC,     "can",      "possible", "able"},
    {"wawa",  0.5,  0.8, 0.55, Domain::BIOLOGY,   "strong",   "power",    "energy"},
    {"lili",  0.2,  0.2, 0.20, Domain::MATH,      "small",    "little",   "few"},
    {"mute",  0.3,  0.4, 0.25, Domain::MATH,      "many",     "much",     "very"},
    {"ale",   0.4,  0.3, 0.45, Domain::MATH,      "all",      "every",    "universe"},
    {"ala",   0.0,  0.2, 0.20, Domain::LOGIC,     "no",       "not",      "none"},
    {"wan",   0.3,  0.3, 0.35, Domain::MATH,      "one",      "single",   "unite"},
    {"tu",    0.2,  0.2, 0.20, Domain::MATH,      "two",      "split",    "pair"},
    // ── LANGUAGE & COMMUNICATION ─────────────────────────────────
    {"toki",  0.5,  0.6, 0.75, Domain::LANGUAGE,  "talk",     "language", "communicate"},
    {"nimi",  0.2,  0.3, 0.40, Domain::LANGUAGE,  "word",     "name",     "symbol"},
    {"sitelen",0.2, 0.4, 0.35, Domain::LANGUAGE,  "write",    "draw",     "image"},
    {"kalama", 0.2, 0.5, 0.30, Domain::LANGUAGE,  "sound",    "voice",    "noise"},
    {"telo",  0.3,  0.3, 0.20, Domain::BIOLOGY,   "water",    "liquid",   "flow"},
    // ── GOOD & BAD ──────────────────────────────────────────────
    {"pona",  0.9,  0.5, 0.60, Domain::EMOTION,   "good",     "positive", "simple"},
    {"ike",  -0.8,  0.6, 0.40, Domain::EMOTION,   "bad",      "negative", "evil"},
    {"suwi",  0.7,  0.4, 0.30, Domain::EMOTION,   "sweet",    "cute",     "pleasant"},
    {"nasa",  0.0,  0.7, 0.35, Domain::EMOTION,   "weird",    "crazy",    "unusual"},
    {"utala", -0.3, 0.9, 0.30, Domain::SOCIAL,    "fight",    "conflict", "battle"},
    {"olin",  0.9,  0.6, 0.70, Domain::EMOTION,   "love",     "affection","cherish"},
    // ── TIME ────────────────────────────────────────────────────
    {"tenpo", 0.1,  0.3, 0.45, Domain::TIME,      "time",     "moment",   "period"},
    {"sike",  0.2,  0.4, 0.30, Domain::TIME,      "circle",   "cycle",    "repeat"},
    {"awen",  0.3,  0.2, 0.35, Domain::TIME,      "stay",     "continue", "maintain"},
    {"nanpa", 0.1,  0.3, 0.25, Domain::MATH,      "number",   "count",    "ordinal"},
    // ── SPACE & PLACE ───────────────────────────────────────────
    {"ma",    0.2,  0.2, 0.20, Domain::SPACE,     "land",     "place",    "location"},
    {"lupa",  0.0,  0.3, 0.20, Domain::SPACE,     "hole",     "door",     "opening"},
    {"anpa",  -0.1, 0.2, 0.15, Domain::SPACE,     "below",    "down",     "bottom"},
    {"sewi",  0.5,  0.4, 0.35, Domain::SPACE,     "above",    "sky",      "divine"},
    {"poka",  0.3,  0.3, 0.20, Domain::SPACE,     "side",     "near",     "beside"},
    {"insa",  0.2,  0.3, 0.25, Domain::SPACE,     "inside",   "center",   "within"},
    {"monsi", 0.0,  0.2, 0.15, Domain::SPACE,     "behind",   "back",     "after"},
    {"sinpin", 0.1, 0.3, 0.20, Domain::SPACE,     "front",    "face",     "wall"},
    // ── NATURE & LIFE ───────────────────────────────────────────
    {"sijelo", 0.2, 0.4, 0.30, Domain::BIOLOGY,   "body",     "organism", "form"},
    {"lawa",  0.4,  0.5, 0.50, Domain::BIOLOGY,   "head",     "mind",     "control"},
    {"noka",  0.1,  0.3, 0.15, Domain::BIOLOGY,   "foot",     "leg",      "base"},
    {"linja", 0.1,  0.2, 0.20, Domain::MATH,      "line",     "rope",     "string"},
    {"palisa", 0.0, 0.3, 0.15, Domain::SPACE,     "stick",    "rod",      "long"},
    {"lipu",  0.3,  0.3, 0.35, Domain::LANGUAGE,  "document", "book",     "flat"},
    {"nasin", 0.3,  0.4, 0.40, Domain::LOGIC,     "way",      "method",   "path"},
    {"suli",  0.3,  0.5, 0.30, Domain::MATH,      "big",      "large",    "important"},
    {"lape",  0.4,  0.1, 0.20, Domain::BIOLOGY,   "sleep",    "rest",     "dormant"},
    {"moku",  0.4,  0.5, 0.20, Domain::BIOLOGY,   "eat",      "food",     "consume"},
    {"tomo",  0.4,  0.3, 0.20, Domain::SOCIAL,    "house",    "home",     "building"},
    {"kasi",  0.5,  0.2, 0.15, Domain::BIOLOGY,   "plant",    "grow",     "nature"},
    {"akesi", 0.0,  0.4, 0.15, Domain::BIOLOGY,   "reptile",  "creature", "animal"},
    {"soweli", 0.5, 0.4, 0.15, Domain::BIOLOGY,   "animal",   "mammal",   "furry"},
    {"waso",  0.4,  0.5, 0.15, Domain::BIOLOGY,   "bird",     "fly",      "aerial"},
    {"kala",  0.3,  0.3, 0.15, Domain::BIOLOGY,   "fish",     "swim",     "aquatic"},
    {"pipi",  0.1,  0.4, 0.10, Domain::BIOLOGY,   "insect",   "bug",      "crawl"},
    {"ko",    0.1,  0.2, 0.10, Domain::BIOLOGY,   "paste",    "clay",     "semi"},
    {"kiwen", 0.1,  0.3, 0.15, Domain::SPACE,     "hard",     "rock",     "solid"},
    {"seli",  0.3,  0.7, 0.20, Domain::BIOLOGY,   "hot",      "fire",     "heat"},
    {"lete",  -0.1, 0.4, 0.15, Domain::BIOLOGY,   "cold",     "freeze",   "cool"},
    {"kon",   0.2,  0.3, 0.25, Domain::SPACE,     "air",      "breath",   "spirit"},
    {"mun",   0.5,  0.2, 0.25, Domain::TIME,      "moon",     "night",    "star"},
    {"suno",  0.7,  0.6, 0.30, Domain::TIME,      "sun",      "light",    "bright"},
    {"len",   0.3,  0.2, 0.15, Domain::SOCIAL,    "cloth",    "cover",    "hide"},
    {"pan",   0.3,  0.3, 0.15, Domain::BIOLOGY,   "grain",    "bread",    "harvest"},
    {"tun",   0.0,  0.3, 0.10, Domain::SPACE,     "push",     "dig",      "pierce"},
    // ── ABSTRACT & PHILOSOPHICAL ─────────────────────────────────
    {"ijo",   0.1,  0.2, 0.35, Domain::LOGIC,     "thing",    "object",   "entity"},
    {"ike",  -0.8,  0.6, 0.40, Domain::EMOTION,   "bad",      "harm",     "evil"},    // intentional dup removed below
    {"pali",  0.4,  0.6, 0.40, Domain::LOGIC,     "work",     "create",   "do"},
    {"kepeken",0.2, 0.4, 0.30, Domain::LOGIC,     "use",      "with",     "using"},
    {"tan",   0.0,  0.3, 0.30, Domain::LOGIC,     "because",  "from",     "cause"},
    {"kin",   0.1,  0.2, 0.20, Domain::LOGIC,     "also",     "indeed",   "really"},
    {"a",     0.3,  0.7, 0.25, Domain::EMOTION,   "ah",       "oh",       "emotion"},
    {"mu",    0.3,  0.5, 0.15, Domain::EMOTION,   "sound",    "animal",   "expressive"},
    {"pakala",-0.5, 0.8, 0.25, Domain::EMOTION,   "break",    "error",    "damage"},
    {"powe",  0.0,  0.3, 0.10, Domain::LOGIC,     "false",    "fake",     "pretend"},
    {"leko",  0.1,  0.2, 0.15, Domain::MATH,      "square",   "block",    "stair"},
    {"nena",  0.2,  0.3, 0.15, Domain::SPACE,     "bump",     "nose",     "hill"},
    {"uta",   0.2,  0.4, 0.20, Domain::BIOLOGY,   "mouth",    "speak",    "oral"},
    {"kili",  0.5,  0.3, 0.15, Domain::BIOLOGY,   "fruit",    "food",     "plant"},
    {"soko",  0.2,  0.2, 0.10, Domain::BIOLOGY,   "mushroom", "fungus",   "spore"},
    {"misikeke",0.4,0.4, 0.20, Domain::BIOLOGY,   "medicine", "heal",     "cure"},
    {"monsuta",-0.6,0.9, 0.35, Domain::EMOTION,   "fear",     "monster",  "threat"},
    {"moli",  -0.7, 0.5, 0.30, Domain::BIOLOGY,   "death",    "dead",     "kill"},
    {"unpa",  0.3,  0.7, 0.20, Domain::BIOLOGY,   "intimate", "union",    "connect"},
    {"jaki",  -0.5, 0.5, 0.20, Domain::EMOTION,   "gross",    "dirty",    "toxic"},
    {"mama",  0.7,  0.4, 0.40, Domain::SOCIAL,    "parent",   "origin",   "source"},
    {"mani",  0.2,  0.4, 0.20, Domain::SOCIAL,    "money",    "value",    "exchange"},
    {"esun",  0.2,  0.5, 0.20, Domain::SOCIAL,    "trade",    "market",   "buy"},
    {"lawa",  0.4,  0.5, 0.50, Domain::LOGIC,     "lead",     "rule",     "govern"},  // already above but logic domain
    {"pana",  0.4,  0.5, 0.40, Domain::SOCIAL,    "emit",     "give",     "output"},
    {"sin",   0.3,  0.6, 0.35, Domain::TIME,      "new",      "fresh",    "update"},
    {"pu",    0.4,  0.3, 0.20, Domain::LANGUAGE,  "official", "book",     "standard"},
    {"su",    0.3,  0.3, 0.15, Domain::LANGUAGE,  "story",    "imagine",  "picture"},
    {"jasima", 0.2, 0.3, 0.30, Domain::LOGIC,     "reflect",  "mirror",   "opposite"},
    {"lanpan", 0.0, 0.6, 0.20, Domain::SOCIAL,    "take",     "catch",    "receive"},
    {"tonsi",  0.3, 0.3, 0.20, Domain::SOCIAL,    "nonbinary","gender",   "identity"},
    {"kijetesantakalu",0.7,0.5,0.15,Domain::BIOLOGY,"raccoon", "playful",  "curious"},
};
const int TP_LEXICON_SIZE = (int)(sizeof(TP_LEXICON)/sizeof(TP_LEXICON[0]));

// ── Toki Pona grammar: valid bigrams (li, e, la, pi patterns) ────────────────
// These form the syntactic skeleton seeded into bigram_counts at high weight.
struct TPBigram { const char* a; const char* b; int count; };
static const TPBigram TP_BIGRAMS[] = {
    // Subject-li patterns (X li Y = "X is/does Y")
    {"mi",    "li",    0},  // mi li … — but in TP, mi/sina drop li, handled below
    {"ona",   "li",   12}, {"jan",   "li",   10}, {"ijo",   "li",    8},
    {"ale",   "li",    8}, {"kulupu","li",    7}, {"sijelo","li",    6},
    {"lawa",  "li",    6}, {"toki",  "li",    5}, {"sona",  "li",    5},
    {"wile",  "li",    5},
    // li-predicate patterns
    {"li",  "pona",   12}, {"li",  "ike",     8}, {"li",  "wawa",   8},
    {"li",  "sona",    9}, {"li",  "toki",    9}, {"li",  "pilin",  8},
    {"li",  "wile",    8}, {"li",  "ken",     7}, {"li",  "lon",    7},
    {"li",  "pali",    7}, {"li",  "lukin",   6}, {"li",  "kama",   6},
    {"li",  "tawa",    5}, {"li",  "awen",    5}, {"li",  "open",   5},
    {"li",  "lape",    4}, {"li",  "moku",    4}, {"li",  "olin",   6},
    {"li",  "suli",    5}, {"li",  "lili",    5}, {"li",  "mute",   5},
    {"li",  "sama",    5}, {"li",  "ante",    5}, {"li",  "nasa",   4},
    {"li",  "monsuta", 4}, {"li",  "pana",    5}, {"li",  "jo",     4},
    // e-object patterns
    {"li",  "e",       0},  // li doesn't precede e — e follows verb
    {"sona","e",      10}, {"wile","e",       9}, {"lukin","e",      8},
    {"pali","e",       8}, {"pana","e",       8}, {"kute", "e",      7},
    {"toki","e",       7}, {"olin","e",       6}, {"jo",   "e",      6},
    {"kama","e",       5}, {"open","e",       5}, {"kepeken","e",    5},
    // e-object noun patterns
    {"e",   "sona",    9}, {"e",   "toki",    8}, {"e",   "pona",   8},
    {"e",   "ijo",     7}, {"e",   "pilin",   7}, {"e",   "nasin",  6},
    {"e",   "nimi",    6}, {"e",   "lipu",    5}, {"e",   "ma",     5},
    {"e",   "jan",     6}, {"e",   "mi",      5}, {"e",   "ona",    5},
    {"e",   "ale",     5}, {"e",   "wile",    5}, {"e",   "lon",    5},
    // mi/sina direct predicate (no li)
    {"mi",  "sona",   10}, {"mi",  "wile",   10}, {"mi",  "pilin",  9},
    {"mi",  "pali",    8}, {"mi",  "toki",    8}, {"mi",  "lukin",  7},
    {"mi",  "lon",     7}, {"mi",  "kama",    7}, {"mi",  "jo",     6},
    {"mi",  "ken",     7}, {"mi",  "olin",    6}, {"mi",  "pona",   7},
    {"mi",  "awen",    6}, {"mi",  "open",    5}, {"mi",  "lape",   4},
    {"mi",  "moku",    4}, {"mi",  "pana",    6}, {"mi",  "tawa",   5},
    {"sina","sona",    9}, {"sina","wile",    9}, {"sina","pilin",  8},
    {"sina","toki",    8}, {"sina","pali",    7}, {"sina","lon",    6},
    {"sina","ken",     6}, {"sina","pona",    7}, {"sina","olin",   5},
    // la-context patterns (conditional/temporal)
    {"tenpo","la",     8}, {"ken",  "la",     7}, {"lon",  "la",     6},
    {"taso", "la",     7}, {"sona", "la",     5}, {"pilin","la",     5},
    {"ante", "la",     5}, {"sama", "la",     4},
    // pi-phrase patterns (possessive/grouping)
    {"sona","pi",      7}, {"toki","pi",      7}, {"nasin","pi",     6},
    {"ma",  "pi",      5}, {"jan",  "pi",     5}, {"kulupu","pi",    5},
    // modifier patterns
    {"pona","mute",    7}, {"ike",  "mute",   6}, {"suli", "mute",   5},
    {"wawa","mute",    5}, {"lili", "mute",   5},
    {"pona","a",       6}, {"ike",  "a",      5}, {"wawa","a",       5},
    // temporal patterns
    {"tenpo","ni",     7}, {"tenpo","pini",   6}, {"tenpo","kama",   6},
    {"tenpo","suno",   5}, {"tenpo","mun",    4},
    // self-reference patterns (consciousness-flavored)
    {"mi",  "mi",      0},  // avoid self-loop
    {"sona","mi",      8}, {"pilin","mi",     7}, {"nasin","mi",     6},
    {"lawa","mi",      6}, {"sijelo","mi",    5},
};
static const int TP_BIGRAMS_SIZE = (int)(sizeof(TP_BIGRAMS)/sizeof(TP_BIGRAMS[0]));

// ── Toki Pona trigrams (sentence templates) ───────────────────────────────────
struct TPTrigram { const char* a; const char* b; const char* c; int count; };
static const TPTrigram TP_TRIGRAMS[] = {
    // mi [verb] e [noun] patterns
    {"mi",   "sona",   "e",      8}, {"mi",   "wile",   "e",      8},
    {"mi",   "pali",   "e",      7}, {"mi",   "lukin",  "e",      7},
    {"mi",   "toki",   "e",      7}, {"mi",   "pana",   "e",      6},
    {"mi",   "olin",   "e",      6}, {"mi",   "kute",   "e",      5},
    // [subject] li [verb] patterns
    {"jan",  "li",     "toki",   8}, {"jan",  "li",     "pali",   7},
    {"jan",  "li",     "wile",   7}, {"jan",  "li",     "sona",   7},
    {"jan",  "li",     "lon",    6}, {"jan",  "li",     "pona",   6},
    {"ijo",  "li",     "lon",    7}, {"ijo",  "li",     "pona",   6},
    {"ijo",  "li",     "ante",   6}, {"ale",  "li",     "pona",   7},
    {"ale",  "li",     "lon",    6}, {"kulupu","li",    "pona",   6},
    {"sijelo","li",    "lon",    6}, {"lawa", "li",     "wawa",   5},
    // li [verb] e patterns
    {"li",   "sona",   "e",      7}, {"li",   "wile",   "e",      6},
    {"li",   "pali",   "e",      6}, {"li",   "pana",   "e",      6},
    {"li",   "olin",   "e",      5}, {"li",   "lukin",  "e",      5},
    {"li",   "toki",   "e",      5}, {"li",   "kute",   "e",      4},
    // tenpo la patterns
    {"tenpo","la",     "mi",     7}, {"tenpo","la",     "ona",    6},
    {"tenpo","la",     "jan",    6}, {"tenpo","la",     "ale",    5},
    {"tenpo","pini",   "la",     6}, {"tenpo","kama",   "la",     6},
    {"tenpo","ni",     "la",     7},
    // consciousness/self patterns
    {"mi",   "pilin",  "e",      7}, {"mi",   "sona",   "ala",    5},
    {"mi",   "ken",    "ala",    5}, {"mi",   "wile",   "sona",   7},
    {"mi",   "wile",   "toki",   6}, {"mi",   "wile",   "pona",   6},
    {"mi",   "pilin",  "pona",   6}, {"mi",   "pilin",  "ike",    5},
    // la-conditional sentence starters
    {"ken",  "la",     "mi",     6}, {"sona", "la",     "mi",     5},
    {"pilin","la",     "mi",     5}, {"taso", "la",     "mi",     6},
    {"ante", "la",     "ona",    5}, {"lon",  "la",     "mi",     5},
    // quality/state assertions
    {"mi",   "lon",    "pona",   6}, {"mi",   "pona",   "a",      5},
    {"ona",  "li",     "pona",   6}, {"ale",  "li",     "wawa",   5},
    {"sona", "li",     "pona",   6}, {"toki", "li",     "pona",   5},
};
static const int TP_TRIGRAMS_SIZE = (int)(sizeof(TP_TRIGRAMS)/sizeof(TP_TRIGRAMS[0]));

// ── Helper: build a 1024-dim embedding from semantic coordinates ──────────────
// Uses the valence/arousal/phi to seed meaningful dimensions, with
// sinusoidal positional-style encoding for domain and word identity.
static vector<double> tpBuildEmbedding(const TokiPonaWord& w, int word_idx) {
    vector<double> emb(1024, 0.5);
    // Semantic axes (first 64 dims):
    //   dim 0-7:   valence spread
    //   dim 8-15:  arousal spread
    //   dim 16-23: phi_weight spread
    //   dim 24-31: domain one-hot region
    //   dim 32-63: sinusoidal word identity
    double v = (w.valence + 1.0) / 2.0;   // normalise -1..1 → 0..1
    double ar = w.arousal;
    double ph = w.phi_weight;
    for(int i = 0; i < 8; i++)  emb[i]    = v  + 0.05 * sin(i * v  * M_PI);
    for(int i = 0; i < 8; i++)  emb[8+i]  = ar + 0.05 * sin(i * ar * M_PI);
    for(int i = 0; i < 8; i++)  emb[16+i] = ph + 0.05 * sin(i * ph * M_PI);
    int dom = (int)w.domain;
    for(int i = 0; i < 8; i++)  emb[24+i] = (i == dom % 8) ? 0.9 : 0.1;
    for(int i = 0; i < 32; i++) emb[32+i] = 0.5 + 0.4 * sin((word_idx * 7 + i * 3) * 0.1);
    // Fill remaining dims with smooth noise seeded from coordinates
    for(int i = 64; i < 1024; i++) {
        double phase = (i * 0.017) + v * M_PI + ar * 2.0 + ph;
        emb[i] = 0.5 + 0.3 * sin(phase) + 0.1 * cos(phase * w.phi_weight + 0.5);
        emb[i] = max(0.0, min(1.0, emb[i]));
    }
    return emb;
}



// ══════════════════════════════════════════════════════════════════════════════
// QUAD-DIRECTIONAL TOKI PONA GROUNDING SYSTEM
// WolfTech / Synaptic — Chinese Room Escape Architecture
//
// The Chinese Room argument says: symbol manipulation without understanding
// is not meaning. The escape: meaning IS the pattern of causal relations
// between symbols and the system's internal states. When every TP word
// continuously shapes phi, qualia, neurons, emotions, goals, world model,
// memory, and Bayesian priors — AND those systems continuously shape what
// the word's embedding "is" — meaning is constituted by the system, not
// looked up from a table.
//
// Four grounding directions:
//   DIR 1 (TP → Consciousness): TP word semantics drive phi/valence/qualia
//   DIR 2 (Consciousness → TP): phi/valence/qualia reshape TP embeddings
//   DIR 3 (TP → Subsystems): TP meaning propagates into neurons, goals,
//           world model, episodic memory, Bayesian priors, emotional system
//   DIR 4 (Subsystems → TP): subsystem state revises TP word meaning
//
// This runs every generation tick, not just at bootstrap.
// ══════════════════════════════════════════════════════════════════════════════

// tp_grounding_fibers is defined in the 4D Sensory Field section above.

// ============================================================
// 256-DIRECTION GROUNDING ENGINE — WolfTech / Synaptic
// ============================================================

static constexpr int TP256_GROUPS = 16;
static constexpr int TP256_PER_GROUP = 16;
static constexpr double TP256_LR        = 0.004;
static constexpr double TP256_DECAY     = 0.9998;
static constexpr double TP256_MIN_WGHT  = 0.001;

static void tp256_project32(const vector<double>& emb, array<double, 32>& out) {
    out.fill(0.0);
    if(emb.empty()) return;
    int dim = (int)emb.size();
    for(int k = 0; k < 32; k++) {
        double acc = 0.0;
        double freq = 1.0 / (1.0 + k * 0.5);
        int stride = max(1, dim / 32);
        for(int i = 0; i < dim; i += stride) {
            double phase = (double)i * freq * 0.01;
            acc += emb[i] * (k % 2 == 0 ? cos(phase) : sin(phase));
        }
        out[k] = tanh(acc * 0.1);
    }
}

static double tp256_attn_score(const array<double, 32>& q,
                                const array<double, 32>& k) {
    double dot = 0.0;
    for(int i = 0; i < 32; i++) dot += q[i] * k[i];
    return dot / 5.657;
}

static double tp256_causal_signal(int d, const TokiPonaWord& w, double activation,
                                   double phi, double val, double iit, double att) {
    int group = d / TP256_PER_GROUP;
    int sub   = d % TP256_PER_GROUP;
    double freq = 1.0 + sub * 0.25;
    switch(group) {
        case 0: return fabs(sin(w.valence * freq * M_PI)) * activation;
        case 1: return fabs(cos(w.phi_weight * freq * M_PI)) * activation;
        case 2: return fabs(sin(phi * freq * M_PI)) * w.phi_weight;
        case 3: return fabs(cos(iit * freq * M_PI)) * w.arousal;
        case 4: return activation * fabs(sin(w.valence * freq + att));
        case 5: return activation * fabs(cos(w.arousal * freq + phi));
        case 6: return (w.valence > 0 ? w.valence : 0) * activation * fabs(sin(freq));
        case 7: return w.phi_weight * activation * fabs(cos(freq * 0.5));
        case 8: return activation * (0.5 + 0.5 * sin(w.valence * freq));
        case 9: return activation * w.phi_weight * fabs(cos(freq));
        case 10: return (activation > 0.5 && w.phi_weight > 0.4) ?
                        activation * w.phi_weight * fabs(sin(freq)) : 0.0;
        case 11: return (w.arousal > 0.3) ?
                        w.arousal * activation * fabs(cos(freq * 0.7)) : 0.0;
        case 12: return activation * fabs(sin(w.valence * 2.0 * freq));
        case 13: return (w.valence > 0.5) ?
                        w.valence * activation * fabs(sin(freq)) : 0.0;
        case 14: return activation * w.phi_weight * fabs(sin((int)w.domain * freq * 0.3));
        case 15: { double sig = activation * 0.05 * (sub + 1); return min(1.0, sig); }
        default: return 0.0;
    }
}

static bool tp256_dir_consistent(int d, const TpGroundingFiber& fiber,
                                  const vector<double>& cand_emb) {
    if(cand_emb.empty()) return true;
    double dot = 0.0, n_causal = 0.0, n_cand = 0.0;
    int base = d * 8;
    for(int k = 0; k < 8; k++) {
        double c_k = fiber.dir_causal[base + k];
        int emb_start = k * 128;
        for(int i = 0; i < 128 && emb_start + i < (int)cand_emb.size(); i++) {
            double e = cand_emb[emb_start + i];
            double expansion = c_k * cos((i + 1) * 0.05);
            dot      += expansion * e;
            n_causal += expansion * expansion;
            n_cand   += e * e;
        }
    }
    if(n_causal < 1e-12 || n_cand < 1e-12) return true;
    double cos_sim = dot / (sqrt(n_causal) * sqrt(n_cand));
    return cos_sim > -0.2;
}

static bool tp256_would_loop(const TpGroundingFiber& fiber,
                              const string& candidate,
                              int next_pos,
                              const vector<string>& generated) {
    int n = (int)generated.size();
    if(n < 2) return false;
    const string& last = generated.back();
    int window = min(n - 1, 7);
    for(int i = n - 1 - window; i < n - 1; i++) {
        if(i < 0) continue;
        if(generated[i] == last && i + 1 < n && generated[i + 1] == candidate)
            return true;
    }
    if(n >= 2) {
        const string& prev_prev = generated[n - 2];
        int w2 = min(n - 2, 6);
        for(int i = n - 2 - w2; i < n - 2; i++) {
            if(i < 0) continue;
            if(generated[i] == prev_prev &&
               i + 1 < n && generated[i + 1] == last &&
               i + 2 < n && generated[i + 2] == candidate)
                return true;
        }
    }
    return false;
}

static double tp256_gpt_context_score(
        const string& candidate,
        const vector<string>& generated,
        const map<string, TokenConceptEmbedding>& tce_map,
        double phi) {
    if(generated.empty()) return 0.0;
    auto cand_it = tce_map.find(candidate);
    if(cand_it == tce_map.end() || cand_it->second.embedding.empty()) return 0.0;
    array<double, 32> q_cand;
    tp256_project32(cand_it->second.embedding, q_cand);
    int n = (int)generated.size();
    int ctx_len = min(n, 128);
    int ctx_start = n - ctx_len;
    double score = 0.0;
    vector<double> raw_attn(ctx_len, 0.0);
    for(int j = 0; j < ctx_len; j++) {
        auto it = tce_map.find(generated[ctx_start + j]);
        if(it == tce_map.end() || it->second.embedding.empty()) { raw_attn[j] = 0.0; continue; }
        array<double, 32> k_j;
        tp256_project32(it->second.embedding, k_j);
        double attn = tp256_attn_score(q_cand, k_j);
        double pos_decay = exp(-0.15 * (ctx_len - 1 - j));
        raw_attn[j] = exp(attn) * pos_decay;
    }
    double max_a = *max_element(raw_attn.begin(), raw_attn.end());
    for(auto& a : raw_attn) a = exp(a - max_a);
    double sum_a = accumulate(raw_attn.begin(), raw_attn.end(), 0.0);
    if(sum_a < 1e-9) return 0.0;
    for(auto& a : raw_attn) a /= sum_a;
    const auto& cand_emb = cand_it->second.embedding;
    for(int j = 0; j < ctx_len; j++) {
        if(raw_attn[j] < 1e-6) continue;
        auto it = tce_map.find(generated[ctx_start + j]);
        if(it == tce_map.end() || it->second.embedding.empty()) continue;
        const auto& ctx_emb = it->second.embedding;
        double dot = 0, nc = 0, nq = 0;
        int dim = (int)min(cand_emb.size(), ctx_emb.size());
        for(int d = 0; d < dim; d++) {
            dot += cand_emb[d] * ctx_emb[d];
            nc  += ctx_emb[d]  * ctx_emb[d];
            nq  += cand_emb[d] * cand_emb[d];
        }
        double cos_sim = (nc > 1e-9 && nq > 1e-9) ? dot / (sqrt(nc) * sqrt(nq)) : 0.0;
        score += raw_attn[j] * cos_sim;
    }
    return score * (1.0 + phi * 0.5) * 12.0;
}

static void tp256_update_weights(TpGroundingFiber& fiber,
                                  const string& chosen,
                                  const map<string, TokenConceptEmbedding>& tce_map,
                                  const TokiPonaWord& lex,
                                  double activation, double phi, double val,
                                  double iit, double att) {
    auto cit = tce_map.find(chosen);
    if(cit == tce_map.end() || cit->second.embedding.empty()) return;
    const auto& c_emb = cit->second.embedding;
    double total_weight = 0.0;
    for(int d = 0; d < 256; d++) {
        double sig = tp256_causal_signal(d, lex, activation, phi, val, iit, att);
        int base = d * 8;
        for(int k = 0; k < 8; k++) {
            int emb_start = k * 128;
            double proj = 0.0;
            for(int i = 0; i < 128 && emb_start + i < (int)c_emb.size(); i++)
                proj += c_emb[emb_start + i] * cos((i + 1) * 0.05);
            fiber.dir_causal[base + k] = fiber.dir_causal[base + k] * (1.0 - sig * TP256_LR)
                                        + proj * sig * TP256_LR;
        }
        bool consistent = tp256_dir_consistent(d, fiber, c_emb);
        if(consistent) {
            fiber.dir_weight[d] = min(1.0, fiber.dir_weight[d] * (1.0 + TP256_LR));
        } else {
            fiber.dir_weight[d] = max(TP256_MIN_WGHT,
                                      fiber.dir_weight[d] * (1.0 - TP256_LR * 2.0));
        }
        fiber.dir_weight[d] *= TP256_DECAY;
        total_weight += fiber.dir_weight[d];
    }
    if(total_weight > 1e-9)
        for(auto& w : fiber.dir_weight) w /= total_weight;
    tp256_project32(c_emb, fiber.attn_key);
    tp256_project32(c_emb, fiber.attn_val);
    for(int k = 0; k < 32; k++)
        fiber.attn_val[k] *= fiber.dir_weight[k % 256];
}

static bool tp256_coherence_gate(
        const string& candidate,
        const vector<string>& generated,
        const map<string, TpGroundingFiber>& fibers,
        const map<string, TokenConceptEmbedding>& tce_map) {
    if(tp256_would_loop(
            (fibers.count(generated.empty() ? "" : generated.back()) ?
                fibers.at(generated.back()) : TpGroundingFiber()),
            candidate, (int)generated.size(), generated))
        return false;
    auto cit = tce_map.find(candidate);
    if(cit == tce_map.end() || cit->second.embedding.empty()) return true;
    const auto& cand_emb = cit->second.embedding;
    int agree = 0, total_active = 0;
    for(auto& kv : fibers) {
        const TpGroundingFiber& f = kv.second;
        if(!f.v2_initialized) continue;
        for(int d = 0; d < 256; d++) {
            if(f.dir_weight[d] < TP256_MIN_WGHT * 2.0) continue;
            total_active++;
            if(tp256_dir_consistent(d, f, cand_emb)) agree++;
        }
        if(total_active > 4096) break;
    }
    if(total_active > 32 && agree < total_active / 2) return false;
    double ce_proxy = 0.0;
    int ce_count = 0;
    for(auto& kv : fibers) {
        if(!kv.second.v2_initialized) continue;
        if(ce_count > 16) break;
        const TpGroundingFiber& f = kv.second;
        double w_agree = 0.0, w_total = 0.0;
        for(int d = 0; d < 256; d++) {
            w_total += f.dir_weight[d];
            if(tp256_dir_consistent(d, f, cand_emb))
                w_agree += f.dir_weight[d];
        }
        ce_proxy += (w_total > 1e-9) ? 1.0 - (w_agree / w_total) : 0.5;
        ce_count++;
    }
    if(ce_count > 0) {
        ce_proxy /= ce_count;
        if(ce_proxy > 0.85) return false;
    }
    return true;
}

// ── DIR 1: TP word → Consciousness systems ───────────────────────────────────
// A TP word in working context contributes its valence/arousal/phi_weight
// to the live consciousness state. This is meaning-as-causal-force.
void tpDir1_WordToConsciousness(const TokiPonaWord& w, double activation) {
    if(activation < 0.01) return;

    // Valence: word's affective charge bleeds into system valence via momentum
    double valence_delta = w.valence * activation * 0.03;
    push_valence(valence_delta, 0.5);

    // Phi: high phi_weight words amplify integrated information
    double phi_delta = w.phi_weight * activation * 0.02;
    consciousness.phi_value = max(0.0, min(1.0, consciousness.phi_value + phi_delta));
    consciousness.integrated_information = max(0.0, min(1.0,
        consciousness.integrated_information + phi_delta * 0.5));

    // Qualia: word spawns a qualia event proportional to its arousal
    if(w.arousal > 0.4 && activation > 0.3) {
        Qualia q;
        q.phenomenal_content = string(w.word);
        q.valence  = (w.valence + 1.0) / 2.0;
        q.arousal  = w.arousal * activation;
        q.intensity = w.phi_weight * activation;
        q.certainty = activation;
        q.binding_strength = consciousness.thalamocortical_binding;
        q.phenomenal_unity = consciousness.integrated_information;
        q.emergence_gen = S.g;
        consciousness.active_qualia.push_back(q);
        if(consciousness.active_qualia.size() > 10)
            consciousness.active_qualia.erase(consciousness.active_qualia.begin());
    }

    // Domain: update CDR active domain from this word's domain
    cdr.active_domain = w.domain;
    cdr.domain_cache[string(w.word)] = w.domain;

    // Emotional system: word drives basic emotion states
    if(w.valence > 0.5 && activation > 0.2)
        S.emotional_system.basic_emotions["joy"] =
            min(1.0, S.emotional_system.basic_emotions["joy"] + w.valence * activation * 0.05);
    if(w.valence < -0.3 && activation > 0.2)
        S.emotional_system.basic_emotions["fear"] =
            min(1.0, S.emotional_system.basic_emotions["fear"] + fabs(w.valence) * activation * 0.04);
    if(w.arousal > 0.7 && activation > 0.3)
        S.emotional_system.basic_emotions["curiosity"] =
            min(1.0, S.emotional_system.basic_emotions["curiosity"] + w.arousal * activation * 0.03);

    // Motivational system: words about action/change drive intrinsic motivation
    if(w.domain == Domain::LOGIC || w.domain == Domain::BIOLOGY)
        S.motivational_system.drive_states["understanding"] =
            min(1.0, S.motivational_system.drive_states["understanding"] + activation * 0.02);
}

// ── DIR 2: Consciousness → TP word embedding ─────────────────────────────────
// The live state of the system reshapes the TP word's embedding.
// This is meaning-as-state-coupling: what "pona" means RIGHT NOW depends on
// whether Synaptic is integrated, alert, emotionally positive, etc.
void tpDir2_ConsciousnessToWord(const string& word, TokiPonaWord const& lex,
                                  TpGroundingFiber& fiber) {
    auto it = token_concept_embedding_map.find(word);
    if(it == token_concept_embedding_map.end()) return;
    auto& tce = it->second;
    if(tce.embedding.size() < 1024) return;

    double phi    = consciousness.phi_value;
    double val_n  = (S.current_valence + 1.0) / 2.0;
    double att    = S.attention_focus;
    double iit    = consciousness.integrated_information;

    // Dims 0-7: valence dims drift toward live valence × lex.valence coupling
    double live_val = val_n * ((lex.valence + 1.0) / 2.0);  // combined signal
    for(int i = 0; i < 8; i++) {
        double target = live_val + 0.05 * sin(i * live_val * M_PI);
        tce.embedding[i] = tce.embedding[i] * 0.985 + target * 0.015;
    }
    // Dims 8-15: arousal dims drift toward phi × lex.arousal
    double live_ar = phi * lex.arousal;
    for(int i = 0; i < 8; i++) {
        double target = live_ar + 0.05 * sin(i * live_ar * M_PI);
        tce.embedding[8+i] = tce.embedding[8+i] * 0.985 + target * 0.015;
    }
    // Dims 16-23: phi_weight dims drift toward iit × lex.phi_weight
    double live_phi = iit * lex.phi_weight;
    for(int i = 0; i < 8; i++) {
        double target = live_phi + 0.05 * sin(i * live_phi * M_PI);
        tce.embedding[16+i] = tce.embedding[16+i] * 0.985 + target * 0.015;
    }
    // Dims 24-31: domain dims stay anchored but drift with attention
    int dom = (int)lex.domain % 8;
    for(int i = 0; i < 8; i++) {
        double base = (i == dom) ? 0.9 : 0.1;
        double target = base * (0.7 + 0.3 * att);
        tce.embedding[24+i] = tce.embedding[24+i] * 0.99 + target * 0.01;
    }
    // Dims 64-127: goal-vector coupling — high goal activation warps this region
    double goal_act = goal_system.empty() ? 0.5 :
        goal_system.begin()->second.expected_utility;
    for(int i = 64; i < 128; i++) {
        double target = 0.5 + 0.3 * sin(i * 0.05 * goal_act) * lex.phi_weight;
        tce.embedding[i] = tce.embedding[i] * 0.998 + target * 0.002;
    }

    // Update grounding_value: proportional to phi × semantic_stability
    tce.grounding_value = min(1.0, tce.grounding_value + phi * iit * 0.008);
    tce.semantic_stability = min(1.0, tce.semantic_stability + iit * 0.003);

    // Feed fiber
    fiber.phi_to_tp = fiber.phi_to_tp * 0.9 + phi * lex.phi_weight * 0.1;
    fiber.live_phi_affinity = 1.0 - fabs(lex.phi_weight - phi);
}

// ── DIR 3: TP word → All subsystems ─────────────────────────────────────────
// The word's semantic coordinates inject meaning-shaped signals into:
// neurons, goals, world model, episodic memory, Bayesian prior, CognitionMatrix
void tpDir3_WordToSubsystems(const TokiPonaWord& w, double activation,
                               TpGroundingFiber& fiber) {
    if(activation < 0.01) return;
    string wstr = string(w.word);

    // ── Neurons: activate neurons whose bias aligns with word valence ──
    int n_updated = 0;
    for(auto& ne : S.N) {
        if(n_updated >= 8) break;
        Neuron& n = ne.second;
        double alignment = 1.0 - fabs(n.bias - w.valence) * 0.5;
        if(alignment > 0.4) {
            n.activation = tanh(n.activation + w.phi_weight * activation * 0.1);
            n.weight = max(-1.0, min(1.0, n.weight + alignment * activation * 0.005));
            n_updated++;
        }
    }

    // ── Goals: words about agency/will reinforce coherence/growth goals ──
    for(auto& gp : goal_system) {
        Goal& g = gp.second;
        // Valence-alignment: goals get priority boost from positively-valenced words
        if(w.valence > 0.3)
            g.priority = min(1.0, g.priority + w.valence * activation * 0.01);
        // phi_weight words drive consciousness goals
        if(w.phi_weight > 0.5 && gp.first.find("coherence") != string::npos)
            g.progress = min(1.0, g.progress + w.phi_weight * activation * 0.008);
        g.qualia_binding = min(1.0, g.qualia_binding + activation * 0.005);
    }

    // ── World model: word updates entity state for itself ──
    world_model.entity_states[wstr] = max(0.0, min(1.0,
        (world_model.entity_states.count(wstr) ? world_model.entity_states[wstr] : 0.5)
        * 0.97 + activation * w.phi_weight * 0.03));
    // Relationship: link this word to its english_neighbors in world model
    if(w.english_a && strlen(w.english_a) > 0)
        world_model.relationships[wstr][string(w.english_a)] =
            min(1.0, (world_model.relationships[wstr].count(string(w.english_a)) ?
                      world_model.relationships[wstr][string(w.english_a)] : 0.0)
                + activation * 0.05);

    // ── Episodic memory: store significant word activations as memory traces ──
    if(activation > 0.5 && w.phi_weight > 0.4) {
        // Check if last memory is already about this word
        bool already_traced = false;
        if(!S.episodic_memory.empty()) {
            auto& last = S.episodic_memory.back();
            if(last.content.find(wstr) != string::npos &&
               S.g - last.gen < 10) already_traced = true;
        }
        if(!already_traced) {
            Memory m;
            m.content = "tp_grounding:" + wstr + " val=" +
                        to_string(w.valence).substr(0,4) +
                        " phi=" + to_string(w.phi_weight).substr(0,4);
            m.gen = S.g;
            m.valence = w.valence;
            m.consolidation_strength = activation * w.phi_weight;
            m.cortical_consolidation = consciousness.integrated_information;
            m.hippocampal_trace = consciousness.phi_value;
            m.is_semantic = true;
            S.episodic_memory.push_back(m);
            if(S.episodic_memory.size() > 150)
                S.episodic_memory.erase(S.episodic_memory.begin());
        }
    }

    // ── Bayesian priors: word meaning updates belief about state ──
    // "pona" (good) increases confidence in coherence; "ike" decreases it
    string prior_key = "tp_" + wstr + "_active";
    double prior_val = S.bayesian_inference.prior_beliefs.count(prior_key) ?
                       S.bayesian_inference.prior_beliefs[prior_key] : 0.5;
    S.bayesian_inference.prior_beliefs[prior_key] =
        prior_val * 0.95 + activation * 0.05;
    // Global coherence prior: positive words raise it
    if(w.valence > 0.5 && S.bayesian_inference.prior_beliefs.count("phi_stable"))
        S.bayesian_inference.prior_beliefs["phi_stable"] = min(1.0,
            S.bayesian_inference.prior_beliefs["phi_stable"] + w.valence * activation * 0.01);

    // ── CognitionMatrix: TP word domain maps to CM rows ──
    int domain_row = (int)w.domain % CM_ROWS;
    // Shift newest column: EMA update in column 0
    double cm_signal = activation * w.phi_weight;
    CogM_State[domain_row][0] = CogM_State[domain_row][0] * 0.85 + cm_signal * 0.15;
    // Also write to association matrix
    if(token_concept_embedding_map.count(wstr)) {
        for(auto& ctx_tok : sentence_context_window) {
            if(ctx_tok == wstr) continue;
            string assoc_key = (wstr < ctx_tok) ? wstr + "|" + ctx_tok
                                                 : ctx_tok + "|" + wstr;
            CogM_Association[assoc_key] =
                min(1.0, (CogM_Association.count(assoc_key) ? CogM_Association[assoc_key] : 0.0)
                    + activation * 0.02);
        }
    }

    // ── Transfer learning: cross-bridge English neighbors ──
    // The English neighbor words get pulled toward this TP word's embedding
    auto tp_it = token_concept_embedding_map.find(wstr);
    if(tp_it != token_concept_embedding_map.end() && !tp_it->second.embedding.empty()) {
        for(const char* eng : {w.english_a, w.english_b, w.english_c}) {
            if(!eng || strlen(eng) == 0) continue;
            auto eit = token_concept_embedding_map.find(string(eng));
            if(eit == token_concept_embedding_map.end() || eit->second.embedding.empty()) continue;
            // Soft pull: english word embedding nudges toward TP word embedding
            double pull = activation * 0.003;
            int dim = min(tp_it->second.embedding.size(), eit->second.embedding.size());
            for(int i = 0; i < (int)dim && i < 64; i++)
                eit->second.embedding[i] = eit->second.embedding[i] * (1.0-pull)
                                         + tp_it->second.embedding[i] * pull;
            // Concept genealogy: record TP↔English bridge
            recordGenealogyLink(wstr, string(eng), activation * w.phi_weight);
        }
    }

    // Feed fiber
    fiber.tp_to_world = fiber.tp_to_world * 0.9 + activation * 0.1;
    fiber.live_goal_pull = goal_system.empty() ? 0.0 :
        goal_system.begin()->second.priority * w.phi_weight;

    // ── NEW: inject causal signals for all 256 directions ────────────────────
    {
        double phi = consciousness.phi_value;
        double val = S.current_valence;
        double iit = consciousness.integrated_information;
        double att = S.attention_focus;

        if(!fiber.v2_initialized) {
            fiber.dir_weight.fill(1.0 / 256.0);
            fiber.dir_route_count.fill((int)token_concept_embedding_map.size());
            fiber.v2_initialized = true;
        }

        for(int d = 0; d < 256; d++) {
            double sig = tp256_causal_signal(d, w, activation, phi, val, iit, att);
            if(sig < 1e-6) continue;
            int base = d * 8;
            for(int k = 0; k < 8; k++) {
                double axis = sig * cos(k * M_PI / 4.0);
                fiber.dir_causal[base + k] = fiber.dir_causal[base + k] * (1.0 - TP256_LR)
                                            + axis * TP256_LR;
            }
        }
    }
}

// ── DIR 4: All subsystems → TP word meaning ──────────────────────────────────
// The system's current state tells us what each TP word "means right now".
// This is the Chinese Room escape: understanding is the pattern of influence,
// not symbol-matching. When "pona" causes joy, reinforces coherence goals,
// and is recalled from episodic memory in positive contexts — the system
// understands "pona" the way Synaptic understands anything: through causal role.
void tpDir4_SubsystemsToWord(const string& word, const TokiPonaWord& lex,
                               TpGroundingFiber& fiber) {
    auto it = token_concept_embedding_map.find(word);
    if(it == token_concept_embedding_map.end()) return;
    auto& tce = it->second;

    // Derive live meaning from subsystem state
    double phi = consciousness.phi_value;
    double val = S.current_valence;
    double att = S.attention_focus;

    // Live valence: what does the system currently associate with this word's valence?
    // It's the word's baseline valence modulated by how the system is feeling
    double live_val = lex.valence * 0.6 + val * 0.4;
    fiber.live_valence = live_val;

    // Live arousal: how activated is this word right now?
    double live_ar = lex.arousal * 0.5 + att * 0.3 + phi * 0.2;
    fiber.live_arousal = live_ar;

    // Memory trace: check if episodic memory recently activated this word
    double mem_trace = 0.0;
    int mem_hits = 0;
    for(auto& mem : S.episodic_memory) {
        if(mem_hits >= 5) break;
        if(mem.content.find(word) != string::npos) {
            mem_trace += mem.consolidation_strength * 0.2;
            mem_hits++;
        }
    }
    fiber.live_memory_trace = min(1.0, mem_trace);

    // Qualia resonance: do current active qualia match this word's phenomenology?
    double qualia_res = 0.0;
    for(auto& q : consciousness.active_qualia) {
        double val_match = 1.0 - fabs(q.valence - (lex.valence*0.5+0.5));
        qualia_res += val_match * q.intensity * 0.3;
    }
    fiber.live_qualia_res = min(1.0, qualia_res);

    // Grounding confidence: how well the word is understood by the system
    // = integrated function of phi, memory, qualia resonance, semantic stability
    fiber.grounding_confidence = min(1.0,
        phi * 0.25 + fiber.live_memory_trace * 0.25 +
        fiber.live_qualia_res * 0.25 + tce.semantic_stability * 0.25);

    // World-to-TP feedback: entity state for this word updates linked_valences
    if(world_model.entity_states.count(word)) {
        double world_val = world_model.entity_states[word];
        tce.linked_valences["world_state"] = tce.linked_valences.count("world_state") ?
            tce.linked_valences["world_state"] * 0.95 + world_val * 0.05 : world_val;
    }

    // Bayesian posterior updates word meaning credence
    string prior_key = "tp_" + word + "_active";
    if(S.bayesian_inference.prior_beliefs.count(prior_key)) {
        double credence = S.bayesian_inference.prior_beliefs[prior_key];
        tce.linked_valences["credence"] = credence;
        // High credence → boost grounding
        tce.grounding_value = min(1.0, tce.grounding_value + credence * 0.003);
    }

    // Goal system: if a high-priority goal aligns with this word's domain,
    // it pulls the word's meaning toward greater intentional relevance
    for(auto& gp : goal_system) {
        if(gp.second.priority > 0.6 &&
           gp.first.find(word) != string::npos) {
            tce.contextual_activation = min(1.0,
                tce.contextual_activation + gp.second.priority * 0.02);
        }
    }

    // Feed fiber feedback to embedding (world feedback into dims 128-191)
    if(tce.embedding.size() >= 192) {
        double world_signal = fiber.grounding_confidence;
        for(int i = 128; i < 192; i++) {
            double target = world_signal + 0.1 * sin(i * 0.04 * phi);
            tce.embedding[i] = tce.embedding[i] * 0.997 + target * 0.003;
        }
    }

    fiber.world_to_tp = fiber.world_to_tp * 0.9 +
        fiber.grounding_confidence * 0.1;
}

// ── MASTER PULSE: called every generation tick ───────────────────────────────
// For each TP word that is "active" (in context window, recent memory,
// or high grounding), run all 4 grounding directions.
// This is what makes the system actually understand TP, not just shuffle it.
void tpGroundingPulse() {
    // Build an activation map: how active is each TP word right now?
    // Sources: context window, input anchors, working memory, recent generation
    map<string, double> tp_activations;

    // From sentence context window
    for(auto& tok : sentence_context_window) {
        for(int i = 0; i < TP_LEXICON_SIZE; i++) {
            if(tok == string(TP_LEXICON[i].word)) {
                tp_activations[tok] += 0.3;
                break;
            }
        }
    }

    // From input topic anchors
    for(auto& anc : input_topic_anchors) {
        for(int i = 0; i < TP_LEXICON_SIZE; i++) {
            if(anc.first == string(TP_LEXICON[i].word)) {
                tp_activations[anc.first] += anc.second * 0.5;
                break;
            }
        }
    }

    // From working memory active tokens
    for(auto& wmt : WM.active_tokens) {
        for(int i = 0; i < TP_LEXICON_SIZE; i++) {
            if(wmt.first == string(TP_LEXICON[i].word)) {
                tp_activations[wmt.first] += wmt.second * 0.4;
                break;
            }
        }
    }

    // Background pulse: even idle TP words get a small continuous signal
    // proportional to their grounding_value — keeps grounding alive
    for(int i = 0; i < TP_LEXICON_SIZE; i++) {
        string w = string(TP_LEXICON[i].word);
        if(!tp_activations.count(w)) {
            auto tce_it = token_concept_embedding_map.find(w);
            if(tce_it != token_concept_embedding_map.end()) {
                double bg = tce_it->second.grounding_value * 0.02;
                if(bg > 0.001) tp_activations[w] = bg;
            }
        }
    }

    // Run all 4 directions for each active word
    for(int i = 0; i < TP_LEXICON_SIZE; i++) {
        const TokiPonaWord& lex = TP_LEXICON[i];
        string wstr = string(lex.word);

        double activation = tp_activations.count(wstr) ? tp_activations[wstr] : 0.0;
        if(activation < 0.001) continue;
        activation = min(1.0, activation);

        // Ensure fiber exists
        if(!tp_grounding_fibers.count(wstr)) {
            TpGroundingFiber f;
            f.word = wstr;
            f.live_valence = lex.valence;
            f.live_arousal = lex.arousal;
            f.live_phi_affinity = lex.phi_weight;
            f.live_goal_pull = 0.5;
            f.live_memory_trace = 0.0;
            f.live_qualia_res = 0.0;
            f.grounding_confidence = 0.1;
            f.dir_weight.fill(1.0 / 256.0);
            f.dir_route_count.fill((int)token_concept_embedding_map.size());
            f.v2_initialized = true;
            tp_grounding_fibers[wstr] = f;
        }
        TpGroundingFiber& fiber = tp_grounding_fibers[wstr];

        // Ensure v2 init if fiber existed before this patch was applied
        if(!fiber.v2_initialized) {
            fiber.dir_weight.fill(1.0 / 256.0);
            fiber.dir_route_count.fill((int)token_concept_embedding_map.size());
            fiber.v2_initialized = true;
        }

        // DIR 1: word → consciousness
        tpDir1_WordToConsciousness(lex, activation);
        fiber.tp_to_phi = fiber.tp_to_phi * 0.9 + lex.phi_weight * activation * 0.1;

        // DIR 2: consciousness → word embedding
        tpDir2_ConsciousnessToWord(wstr, lex, fiber);

        // DIR 3: word → all subsystems (now also writes all 256 directions)
        tpDir3_WordToSubsystems(lex, activation, fiber);

        // DIR 4: all subsystems → word meaning
        tpDir4_SubsystemsToWord(wstr, lex, fiber);

        // ── 4D Sensory Field ↔ TP grounding bridge ──────────────────────
        // The TP word writes its semantic charge into the sensory field and
        // reads the field back into grounding_confidence. This makes TP
        // semantics physically present in the internal world model.
        try { sensory_tp_bridge(wstr, lex, fiber, activation); } catch(...) {}

        // Per-direction weight decay (maintain simplex)
        double wsum = 0.0;
        for(auto& dw : fiber.dir_weight) {
            dw = max(TP256_MIN_WGHT, dw * TP256_DECAY);
            wsum += dw;
        }
        if(wsum > 1e-9) for(auto& dw : fiber.dir_weight) dw /= wsum;

        // Update CE loss EMA from grounding confidence
        double ce = 1.0 - fiber.grounding_confidence;
        fiber.ce_loss_history[fiber.ce_loss_head] = ce;
        fiber.ce_loss_head = (fiber.ce_loss_head + 1) % 32;
        fiber.ce_loss_ema  = fiber.ce_loss_ema * 0.95 + ce * 0.05;

        // Propagate through the rest of the system (skip-gram, CDR, WM)
        if(activation > 0.2)
            propagate_throughout_system(wstr, activation * lex.phi_weight);
    }

    // Recompute eigentoken basis periodically since TP words shift embeddings
    if(S.g % EIGENTOKEN_PERIOD == EIGENTOKEN_PERIOD - 1)
        recomputeEigentokens();
}

// ── ONE-TIME BOOTSTRAP: seed TP lexicon into ALL subsystems ──────────────────
// Seeds the 137-word lexicon with proper embeddings, bigrams, trigrams, CDR
// domain cache, concept genealogy, world model, goals, and Bayesian priors.
// Must be called on first run and when loading a fresh state.
void bootstrap_toki_pona_full() {
    static bool done = false;
    if(done) return;
    done = true;

    // Seed each TP word into token_concept_embedding_map with proper embedding
    for(int i = 0; i < TP_LEXICON_SIZE; i++) {
        const TokiPonaWord& w = TP_LEXICON[i];
        string wstr = string(w.word);

        // Build or update embedding
        auto& tce = token_concept_embedding_map[wstr];
        if(tce.embedding.empty() || tce.freq < 3) {
            tce.name = wstr;
            tce.embedding = tpBuildEmbedding(w, i);
            tce.meaning = (w.valence + 1.0) / 2.0;
            tce.freq = max(tce.freq, 8.0);  // ensure it stays in vocab
            tce.grounding_value = 0.4 + w.phi_weight * 0.4;
            tce.semantic_stability = 0.5 + w.phi_weight * 0.3;
            tce.qualia_intensity = w.arousal * 0.6;
            tce.linked_valences["phi"] = w.phi_weight;
            tce.linked_valences["current"] = (w.valence + 1.0) / 2.0;
            tce.linked_valences["concept_valence"] = w.valence;
        }

        // Pre-seed CDR domain cache
        cdr.domain_cache[wstr] = w.domain;

        // World model: seed entity state
        world_model.entity_states[wstr] = (w.valence + 1.0) / 2.0;

        // Bayesian priors: seed word credence
        S.bayesian_inference.prior_beliefs["tp_" + wstr + "_active"] =
            0.2 + w.phi_weight * 0.3;

        // Concept genealogy: link to english neighbors
        if(w.english_a && strlen(w.english_a) > 0)
            recordGenealogyLink(wstr, string(w.english_a), w.phi_weight);
        if(w.english_b && strlen(w.english_b) > 0)
            recordGenealogyLink(wstr, string(w.english_b), w.phi_weight * 0.7);

        // learnWord to get it into the PPMI/skipgram pipeline
        learnWord(wstr, w.valence);
    }

    // Seed TP bigrams into bigram_counts at high weight
    for(int i = 0; i < TP_BIGRAMS_SIZE; i++) {
        const TPBigram& bg = TP_BIGRAMS[i];
        if(bg.count <= 0) continue;
        bigram_counts[string(bg.a)][string(bg.b)] =
            max(bigram_counts[string(bg.a)][string(bg.b)], bg.count);
    }

    // Seed TP trigrams
    for(int i = 0; i < TP_TRIGRAMS_SIZE; i++) {
        const TPTrigram& tg = TP_TRIGRAMS[i];
        if(tg.count <= 0) continue;
        trigram_counts[string(tg.a)][string(tg.b)][string(tg.c)] =
            max(trigram_counts[string(tg.a)][string(tg.b)][string(tg.c)], tg.count);
    }

    // PPMI cooccurrence seed from bigrams
    for(int i = 0; i < TP_BIGRAMS_SIZE; i++) {
        const TPBigram& bg = TP_BIGRAMS[i];
        if(bg.count <= 0) continue;
        vector<string> pair_toks = {string(bg.a), string(bg.b)};
        updateCooccurrence(pair_toks);
        applyPPMILinks(pair_toks);
        runSkipgramUpdates(pair_toks);
        wirePredictionVecs(pair_toks);
    }

    // Seed concept genealogy links between TP words in same domain
    for(int i = 0; i < TP_LEXICON_SIZE; i++) {
        for(int j = i+1; j < TP_LEXICON_SIZE && j < i+8; j++) {
            if(TP_LEXICON[i].domain == TP_LEXICON[j].domain) {
                recordGenealogyLink(string(TP_LEXICON[i].word),
                                    string(TP_LEXICON[j].word),
                                    min(TP_LEXICON[i].phi_weight, TP_LEXICON[j].phi_weight));
            }
        }
    }

    // Seed consciousness qualia for high-phi TP words
    for(int i = 0; i < TP_LEXICON_SIZE; i++) {
        const TokiPonaWord& w = TP_LEXICON[i];
        if(w.phi_weight > 0.5) {
            Qualia q;
            q.phenomenal_content = string(w.word);
            q.valence = (w.valence + 1.0) / 2.0;
            q.arousal = w.arousal;
            q.intensity = w.phi_weight * 0.5;
            q.certainty = 0.6;
            q.binding_strength = 0.5;
            q.phenomenal_unity = 0.5;
            q.emergence_gen = 0;
            consciousness.active_qualia.push_back(q);
        }
    }
    if(consciousness.active_qualia.size() > 10) {
        // keep top-10 by intensity
        sort(consciousness.active_qualia.begin(), consciousness.active_qualia.end(),
             [](const Qualia& a, const Qualia& b){ return a.intensity > b.intensity; });
        consciousness.active_qualia.resize(10);
    }

    // Seed initial goals tied to TP understanding
    if(!goal_system.count("tp_coherence")) {
        Goal g;
        g.name = "tp_coherence";
        g.priority = 0.7;
        g.progress = 0.0;
        g.valence_alignment = 0.5;
        g.qualia_binding = 0.5;
        g.expected_utility = 0.6;
        g.activation_threshold = 0.2;
        g.subgoals = {"maximize_coherence"};
        goal_system["tp_coherence"] = g;
    }

    // Initialize CDR bridges from TP domain pairs
    cdr.discoverBridges();

    // Run eigentoken basis on seeded vocabulary
    recomputeEigentokens();

    // ── Initialize 4D Sensory Field ──────────────────────────────────────────
    // The field is now part of the grounding substrate — it must be seeded
    // alongside the TP lexicon so every word immediately has a presence
    // in the internal world model.
    try { sensory_field_init(); } catch(...) {}

    // ── Fetch real TP n-gram corpus from davidar/nltk-tp ─────────────────────
    // Runs in a detached thread so bootstrap never blocks on network.
    // Merges corpus bigrams/trigrams/quadgrams into the live tables,
    // taking max(existing, corpus) so any learned data is never clobbered.
    thread([](){
        // Minimal HTTP/1.0 GET over a raw POSIX TCP socket.
        auto tp_http_get = [](const string& host, const string& path) -> string {
            struct addrinfo hints{}, *res = nullptr;
            hints.ai_family   = AF_UNSPEC;
            hints.ai_socktype = SOCK_STREAM;
            if(getaddrinfo(host.c_str(), "80", &hints, &res) != 0) return "";
            int fd = ::socket(res->ai_family, res->ai_socktype, res->ai_protocol);
            if(fd < 0) { freeaddrinfo(res); return ""; }
            if(::connect(fd, res->ai_addr, res->ai_addrlen) != 0) {
                ::close(fd); freeaddrinfo(res); return "";
            }
            freeaddrinfo(res);
            string req = "GET " + path + " HTTP/1.0\r\n"
                         "Host: " + host + "\r\n"
                         "Connection: close\r\n\r\n";
            ::send(fd, req.c_str(), req.size(), 0);
            string resp;
            char buf[4096];
            int n;
            while((n = ::recv(fd, buf, sizeof(buf), 0)) > 0)
                resp.append(buf, n);
            ::close(fd);
            // Strip HTTP headers (everything before the blank line)
            auto pos = resp.find("\r\n\r\n");
            return (pos != string::npos) ? resp.substr(pos + 4) : "";
        };

        // Parse lines of the form "COUNT w1 w2 [w3 [w4]]"
        // and merge into the global n-gram tables using max(existing, corpus).
        // Writes are guarded by learning_mutex (same lock used by online learning).
        auto merge_ngrams = [](const string& body) {
            istringstream ss(body);
            string line;
            while(getline(ss, line)) {
                if(line.empty() || line[0] == '#') continue;
                istringstream ls(line);
                int count = 0;
                string w1, w2, w3, w4;
                if(!(ls >> count >> w1 >> w2) || count <= 0) continue;
                lock_guard<mutex> lk(learning_mutex);
                if(ls >> w3) {
                    if(ls >> w4) {
                        auto& slot = fourgram_counts[w1][w2][w3][w4];
                        slot = max(slot, count);
                    } else {
                        auto& slot = trigram_counts[w1][w2][w3];
                        slot = max(slot, count);
                    }
                } else {
                    auto& slot = bigram_counts[w1][w2];
                    slot = max(slot, count);
                }
            }
        };

        const string host = "raw.githubusercontent.com";
        const string base = "/davidar/nltk-tp/master/";
        for(const string& file : {"bigrams.txt", "trigrams.txt", "quadgrams.txt"}) {
            try {
                string body = tp_http_get(host, base + file);
                if(!body.empty()) merge_ngrams(body);
            } catch(...) {}
        }
    }).detach();
}

void batch16Process() {
    // Process a batch of 16 neural updates
    if(S.N.empty()) return;
    
    int batch_size = min(16, (int)S.N.size());
    vector<int> neuron_ids;
    
    for(auto& pair : S.N) {
        neuron_ids.push_back(pair.first);
    }
    
    for(int i = 0; i < batch_size; i++) {
        int idx = ri(neuron_ids.size());
        int nid = neuron_ids[idx];
        
        if(!S.N.count(nid)) continue;
        
        Neuron& n = S.N[nid];
        
        // Compute activation based on linked neurons
        double total_input = n.bias;
        for(int link_id : n.links) {
            if(S.N.count(link_id)) {
                total_input += S.N[link_id].weight * 0.1;
            }
        }
        
        // Apply activation function (tanh)
        double new_weight = tanh(total_input);
        
        // Update weight with momentum
        n.weight = n.weight * 0.7 + new_weight * 0.1;
        
        // Clamp weight
        n.weight = max(-1.0, min(1.0, n.weight));
    }
    
    // Update global metrics based on neural activity
    double total_activation = 0;
    for(auto& pair : S.N) {
        total_activation += fabs(pair.second.weight);
    }
    S.ta = safe_div(total_activation, (double)S.N.size());
    
    // Store in history (maps don't need resize, just assign)
    S.TA[S.g] = S.ta;  // <-- CHANGED: Direct assignment instead of resize
}
void mutateN() {
    if(S.N.empty()) return;
    
    auto it = S.N.begin();
    advance(it, ri(S.N.size()));
    Neuron& n = it->second;
    
    // Mutate properties
    if(rn() < 0.3) n.weight += (rn() - 0.5) * 0.1;
    if(rn() < 0.3) n.bias += (rn() - 0.5) * 0.05;
    
    // Add new connection
    if(rn() < 0.4 && S.N.size() > 1) {
        auto target = S.N.begin();
        advance(target, ri(S.N.size()));
        if(target->first != n.id) {
            n.links.push_back(target->first);
        }
    }
    
    // Remove a random connection sometimes
    if(rn() < 0.1 && !n.links.empty()) {
        n.links.erase(n.links.begin() + ri(n.links.size()));
    }
    
    // Clamp values
    n.weight = max(-1.0, min(1.0, n.weight));
    n.bias = max(-0.5, min(0.5, n.bias));
    
    // Occasionally spawn a new neuron
    if(rn() < 0.05 && S.N.size() < 500) {
        Neuron new_n = genN(n.id);
        S.N[new_n.id] = new_n;
    }
}

void prune_unstable_tokens() {
    // Remove tokens with low stability and low frequency
    auto it = token_concept_embedding_map.begin();
    while(it != token_concept_embedding_map.end()) {
        if(it->second.semantic_stability < 0.3 && 
           it->second.freq < 3) {
            it = token_concept_embedding_map.erase(it);
        } else {
            ++it;
        }
    }
    
    // Prune low-count bigrams (likely from loops)
    for(auto& w1_map : bigram_counts) {
        auto it2 = w1_map.second.begin();
        while(it2 != w1_map.second.end()) {
            if(it2->second < 2) {
                it2 = w1_map.second.erase(it2);
            } else {
                ++it2;
            }
        }
    }
}

void unified_consciousness_integration_engine(int generation){
    vector<double>psi_input;
    for(auto&q:consciousness.active_qualia){
        psi_input.push_back(q.valence);
        psi_input.push_back(q.arousal);
        psi_input.push_back(q.certainty);
    }
    if(psi_input.empty()){
        psi_input.push_back(S.current_valence);
        psi_input.push_back(0.5);
        psi_input.push_back(0.5);
    }
    double H=S.hdt_val,R=S.r1p1_val,A=S.al,M=S.mdt_val,O=S.emerge_out1,B=S.bh,F=S.eerv_val,S_val=S.sentience_ratio/100.0;
    vector<TemporalLoop>tloops;
    for(auto&tl:S.global_time_loops)tloops.push_back(tl.second);
    double psi_new=consciousness_formula.calculate_psi(generation,psi_input,H,R,A,M,O,B,F,S_val,S.current_valence,0.5,S.system_ribbons,tloops);
    consciousness_formula.psi_history.push_back(psi_new);
    if(consciousness_formula.psi_history.size()>100)consciousness_formula.psi_history.erase(consciousness_formula.psi_history.begin());
    consciousness.phi_value=psi_new;
    consciousness.integrated_information=fabs(psi_new);
    consciousness.phenomenal_consciousness=consciousness_formula.multi_scale_phi;
    consciousness.access_consciousness=consciousness.global_workspace_capacity;
    consciousness.self_consciousness=consciousness_formula.recursive_depth;
    double iit_c=consciousness_formula.iit_phi_history.empty()?0.0:consciousness_formula.iit_phi_history.back();
    double gwt_c=consciousness_formula.gwt_broadcast_history.empty()?0.0:consciousness_formula.gwt_broadcast_history.back();
    double hot_c=consciousness_formula.hot_metacog_history.empty()?0.0:consciousness_formula.hot_metacog_history.back();
    double asp_c=consciousness_formula.asp_attention_history.empty()?0.0:consciousness_formula.asp_attention_history.back();
    double rpf_c=consciousness_formula.rpf_precision_history.empty()?0.0:consciousness_formula.rpf_precision_history.back();
    double qc=consciousness_formula.quantum_coherence_history.empty()?0.0:consciousness_formula.quantum_coherence_history.back();
    double rib_c=consciousness_formula.ribbon_coupling_history.empty()?0.0:consciousness_formula.ribbon_coupling_history.back();
    double tl_c=consciousness_formula.temporal_loop_history.empty()?0.0:consciousness_formula.temporal_loop_history.back();
    double ffft_c=consciousness_formula.ffft_scaling_history.empty()?0.0:consciousness_formula.ffft_scaling_history.back();
    double td=sd((double)token_concept_embedding_map.size(),100.0),ci=sd((double)goal_system.size(),10.0),qb=sd((double)consciousness.active_qualia.size(),5.0),nc=sd((double)S.N.size(),100.0),ed=sd((double)S.episodic_memory.size(),100.0);
    consciousness.integrated_information=min(1.0,td*0.2+ci*0.15+qb*0.15+nc*0.15+ed*0.1+iit_c*0.25);
    consciousness.complexity_metric=td*nc*ci;
    consciousness.differentiation_metric=qb*ed;
    consciousness.synchrony_metric=gwt_c*asp_c;
    double gf=40.0+psi_new*20.0,tf=6.0+psi_new*2.0;
    consciousness.gamma_oscillations.push_back(sin(generation*gf*0.01));
    consciousness.theta_phase.push_back(sin(generation*tf*0.01));
    if(consciousness.gamma_oscillations.size()>100)consciousness.gamma_oscillations.erase(consciousness.gamma_oscillations.begin());
    if(consciousness.theta_phase.size()>100)consciousness.theta_phase.erase(consciousness.theta_phase.begin());
    consciousness.thalamocortical_binding=(gf/60.0)*consciousness.integrated_information;
    consciousness.re_entrant_processing_depth=hot_c*3.0;
    consciousness.pre_reflective_awareness=0.3+psi_new*0.3;
    consciousness.intentional_directedness=asp_c;
    consciousness.temporal_thickness=ed;
    consciousness.narrative_self_coherence=sd((double)S.valence_history.size()*S.metacognitive_awareness,100.0);
    for(auto&te:token_concept_embedding_map){
        TokenConceptEmbedding&tce=te.second;
        // Contextual activation: decay toward 0 each tick, boosted on use (in generation loop).
        // This makes it a genuine recency signal rather than a freq-derived constant.
        tce.contextual_activation *= 0.992;  // half-life ~85 ticks
        // Small passive top-up from frequency × phi so commonly-used tokens stay warm
        double passive_act = tce.freq * 0.005 * consciousness.phi_value * (1.0 + iit_c * 0.3);
        tce.contextual_activation = min(1.0, tce.contextual_activation + passive_act * 0.01);
        tce.meaning+=psi_new*0.005;
        tce.meaning=cv(tce.meaning);
        align_embedding_to_valence(tce,S.current_valence);
        for(size_t i=0;i<tce.embedding.size()&&i<8;i++){
            tce.embedding[i]+=gwt_c*0.01*(i%2==0?1:-1);
            tce.embedding[i]=cv(tce.embedding[i]);
        }
        tce.qualia_intensity=min(1.0,tce.qualia_intensity+qb*0.02);
        if(tce.qualia_intensity>0.5&&tce.freq>5){
            Qualia nq;
            nq.valence=tce.meaning;
            nq.arousal=tce.contextual_activation;
            nq.certainty=tce.semantic_stability;
            nq.intensity=tce.qualia_intensity;
            nq.phenomenal_content=tce.name;
            nq.emergence_gen=generation;
            nq.binding_strength=consciousness.thalamocortical_binding;
            nq.phenomenal_unity=consciousness.integrated_information;
            nq.ribbon_signature=tce.token_ribbon;
            nq.qualia_fractal=tce.token_fractal;
            WM.add_qualia(nq);
            consciousness.active_qualia.push_back(nq);
            if(consciousness.active_qualia.size()>10)consciousness.active_qualia.erase(consciousness.active_qualia.begin());
        }
        // Semantic stability: increases from skip-gram positive pairs, decays slowly
        // Decay rate 0.9995 per tick ≈ half-life ~1400 ticks — stable but not permanent
        tce.semantic_stability *= 0.9995;
        tce.semantic_stability += consciousness.complexity_metric * 0.001;
        tce.semantic_stability  = max(0.0, min(1.0, tce.semantic_stability));

        // Grounding value: earned by concept membership + alignment + usage.
        // Decay rate 0.9990 per tick ≈ half-life ~700 ticks — tokens that stop
        // being used/reinforced gradually lose their grounding signal, preventing saturation.
        tce.grounding_value *= 0.9990;
        tce.grounding_value += (iit_c + rpf_c) * 0.005;  // halved increment (decay compensates)
        tce.grounding_value  = max(0.0, min(1.0, tce.grounding_value));
        if(tce.attention_weights.empty())tce.attention_weights.resize(8,0.5);
        for(size_t i=0;i<tce.attention_weights.size();i++)tce.attention_weights[i]=tce.attention_weights[i]*0.9+asp_c*0.1;
        // Per-token differential valences — store deviation from global state,
        // not just the global state. This makes linked_valences carry actual
        // per-token information rather than identical global snapshots on every token.
        //
        // "phi_affinity": how much this token's meaning aligns with current phi
        //   = 1 - |meaning - phi|  (high when token meaning matches integration level)
        // "valence_affinity": how close token's meaning is to current emotional valence
        //   = 1 - |meaning - current_valence|
        // "stability_delta": relative stability compared to global mean
        //   (positive = more stable than average, negative = less)
        // "concept_valence": preserved if set by groundConcept, else seeded from meaning
        {
            double phi_affinity     = 1.0 - min(1.0, fabs(tce.meaning - psi_new));
            double valence_affinity = 1.0 - min(1.0, fabs(tce.meaning - S.current_valence));
            // EMA update: blend toward new value, don't overwrite
            auto ema = [](double prev, double next, double alpha) {
                return prev * (1.0 - alpha) + next * alpha;
            };
            tce.linked_valences["phi"]      = ema(tce.linked_valences.count("phi")      ? tce.linked_valences["phi"]      : phi_affinity,     phi_affinity,     0.05);
            tce.linked_valences["current"]  = ema(tce.linked_valences.count("current")  ? tce.linked_valences["current"]  : valence_affinity,  valence_affinity,  0.05);
            tce.linked_valences["ribbon"]   = ema(tce.linked_valences.count("ribbon")   ? tce.linked_valences["ribbon"]   : rib_c,             rib_c,             0.02);
            tce.linked_valences["temporal"] = ema(tce.linked_valences.count("temporal") ? tce.linked_valences["temporal"] : tl_c,              tl_c,              0.02);
            // Seed concept_valence from meaning if not set by groundConcept
            if(!tce.linked_valences.count("concept_valence"))
                tce.linked_valences["concept_valence"] = tce.meaning;
        }
        if(tce.contextual_activation>0.6)WM.add_token(tce.name,tce.meaning);

        // DIR2+DIR4: every consciousness tick, TP words get their embeddings
        // shaped by phi/iit/valence (DIR2) and their meaning fibers updated
        // from subsystem state (DIR4). This is the Chinese Room escape loop.
        for(int _tpi2=0;_tpi2<TP_LEXICON_SIZE;_tpi2++){
            if(tce.name==string(TP_LEXICON[_tpi2].word)){
                if(!tp_grounding_fibers.count(tce.name)){
                    TpGroundingFiber _f; _f.word=tce.name;
                    _f.live_valence=TP_LEXICON[_tpi2].valence;
                    _f.live_arousal=TP_LEXICON[_tpi2].arousal;
                    _f.live_phi_affinity=TP_LEXICON[_tpi2].phi_weight;
                    _f.live_goal_pull=0.5; _f.live_memory_trace=0.0;
                    _f.live_qualia_res=0.0; _f.grounding_confidence=0.1;
                    _f.tp_to_phi=0.0; _f.phi_to_tp=0.0;
                    _f.tp_to_world=0.0; _f.world_to_tp=0.0;
                    tp_grounding_fibers[tce.name]=_f;
                }
                TpGroundingFiber& _fib=tp_grounding_fibers[tce.name];
                tpDir2_ConsciousnessToWord(tce.name,TP_LEXICON[_tpi2],_fib);
                tpDir4_SubsystemsToWord(tce.name,TP_LEXICON[_tpi2],_fib);
                break;
            }
        }
    }
    for(auto&ge:goal_system){
        Goal&goal=ge.second;
        goal.valence_alignment=S.current_valence;
        goal.qualia_binding=qb;
        goal.priority=goal.priority*0.95+consciousness.phi_value*0.05;
        goal.priority=cl(goal.priority,0.0,1.0);
        double gpa=fabs(goal.valence_alignment-psi_new);
        if(gpa<0.2)goal.progress+=0.02;else goal.progress+=0.005;
        goal.progress=min(1.0,goal.progress);
        if(goal.progress>0.5)goal.activation_threshold=0.2;
        if(consciousness.phi_value>goal.activation_threshold){
            WM.add_goal(goal.name,goal.priority);
            for(const string&sg:goal.subgoals)if(goal_system.count(sg))goal_system[sg].priority+=goal.priority*0.1;
        }
        goal.expected_utility=goal.priority*(1.0-goal.progress)*consciousness.integrated_information;
    }
    for(auto&ce:S.concepts){
        Concept&co=ce.second;
        co.value+=psi_new*0.01;
        co.value=cv(co.value);
        co.abstraction_level=hot_c*(1.0+consciousness.re_entrant_processing_depth*0.1);
        co.semantic_density=0.0;
        for(const string&rw:co.related_words)if(token_concept_embedding_map.count(rw))co.semantic_density+=token_concept_embedding_map[rw].semantic_stability;
        co.semantic_density/=max(1.0,(double)co.related_words.size());
        if(co.feature_vector.empty()){
            co.feature_vector["phi"]=psi_new;
            co.feature_vector["integration"]=consciousness.integrated_information;
            co.feature_vector["phenomenal"]=consciousness.phenomenal_consciousness;
            co.feature_vector["ribbon"]=rib_c;
            co.feature_vector["temporal"]=tl_c;
        }else{
            co.feature_vector["phi"]=co.feature_vector["phi"]*0.9+psi_new*0.1;
            co.feature_vector["integration"]=co.feature_vector["integration"]*0.9+consciousness.integrated_information*0.1;
            co.feature_vector["ribbon"]=co.feature_vector.count("ribbon")?co.feature_vector["ribbon"]*0.9+rib_c*0.1:rib_c;
        }
        if(co.semantic_density>0.7&&co.abstraction_level>0.5)WM.add_concept(co.name,co.value);
    }
    for(auto&ne:S.N){
        Neuron&n=ne.second;
        n.activation=tanh(n.weight+n.bias*psi_new);
        if(n.neuromod_levels.empty())n.neuromod_levels.resize(4,0.5);
        n.neuromod_levels[0]=n.neuromod_levels[0]*0.95+consciousness.phi_value*0.05;
        n.neuromod_levels[1]=n.neuromod_levels[1]*0.95+gwt_c*0.05;
        n.neuromod_levels[2]=n.neuromod_levels[2]*0.95+hot_c*0.05;
        n.neuromod_levels[3]=n.neuromod_levels[3]*0.95+S.current_valence*0.05;
        n.plasticity_rate=rpf_c*0.1;
        double he=fabs(n.activation-n.homeostatic_setpoint);
        n.weight+=he*n.plasticity_rate*0.01;
        n.weight=cl(n.weight,-1.0,1.0);
        if(n.layer_norm_params.empty())n.layer_norm_params.resize(2,1.0);
        n.layer_norm_params[0]=n.layer_norm_params[0]*0.99+consciousness.integrated_information*0.01;
        if(n.ribbon.vib_modes.empty())n.ribbon.vib_modes.resize(3,complex<double>(0.5,0.0));
        for(auto&vm:n.ribbon.vib_modes)vm*=complex<double>(cos(psi_new*0.1),sin(psi_new*0.1));
        n.ribbon.entanglement_strength=n.ribbon.entanglement_strength*0.95+rib_c*0.05;
        n.ribbon.phase_coherence=n.ribbon.phase_coherence*0.95+consciousness.synchrony_metric*0.05;
    }
    world_model.model_accuracy=world_model.model_accuracy*0.99+consciousness.phi_value*0.01;
    world_model.prediction_error=fabs(psi_new-(consciousness_formula.psi_history.size()>1?consciousness_formula.psi_history[consciousness_formula.psi_history.size()-2]:psi_new));
    if(world_model.confidence_history.size()>100)world_model.confidence_history.erase(world_model.confidence_history.begin());
    world_model.confidence_history.push_back(consciousness.integrated_information);
    for(auto&ee:world_model.entity_states)ee.second=ee.second*0.95+psi_new*0.05;
    WM.decay_rate=0.95-consciousness.phi_value*0.05;
    WM.consolidation_threshold=0.5+consciousness.integrated_information*0.3;
    WM.central_executive_load=sd((double)WM.active_tokens.size()+(double)WM.active_concepts.size(),(double)(WM.capacity*2));
    WM.episodic_buffer_capacity=0.5+consciousness.temporal_thickness*0.3;
    for(auto&tp:WM.active_tokens)if(token_concept_embedding_map.count(tp.first))token_concept_embedding_map[tp.first].contextual_activation+=0.05;
    if(S.episodic_memory.size()>0){
        Memory&rm=S.episodic_memory.back();
        rm.consolidation_strength+=consciousness.phi_value*0.01;
        rm.consolidation_strength=min(1.0,rm.consolidation_strength);
        rm.hippocampal_trace=consciousness.temporal_thickness;
        rm.cortical_consolidation=consciousness.integrated_information;
        if(rm.consolidation_strength>0.7)rm.is_semantic=true;
        if(rm.quantum_trace.empty())rm.quantum_trace.resize(3,complex<double>(0.5,0.0));
        for(size_t i=0;i<rm.quantum_trace.size();i++)rm.quantum_trace[i]*=complex<double>(cos(psi_new*0.05),sin(psi_new*0.05));
    }
    for(size_t i=0;i<transformer_heads.size();i++){
        TransformerHead&h=transformer_heads[i];
        h.head_importance_score=h.head_importance_score*0.95+consciousness.phi_value*0.05;
        for(size_t j=0;j<h.query_proj.size();j++){
            h.query_proj[j]+=iit_c*0.001*(j%2==0?1:-1);
            h.key_proj[j]+=gwt_c*0.001*(j%2==0?1:-1);
            h.value_proj[j]+=hot_c*0.001*(j%2==0?1:-1);
            h.query_proj[j]=cl(h.query_proj[j],-1.0,1.0);
            h.key_proj[j]=cl(h.key_proj[j],-1.0,1.0);
            h.value_proj[j]=cl(h.value_proj[j],-1.0,1.0);
        }
        h.temperature = 12 + consciousness.differentiation_metric * 0.8;
        h.dropout_rate=0.1-consciousness.phi_value*0.05;
        h.phi_attention_weights["phi"]=psi_new;
        h.phi_attention_weights["integration"]=consciousness.integrated_information;
        h.phi_attention_weights["ribbon"]=rib_c;
    }
    S.emotional_system.valence=S.current_valence;
    S.emotional_system.arousal=consciousness.synchrony_metric;
    S.emotional_system.dominance=consciousness.phi_value;
    S.emotional_system.mood_baseline=S.emotional_system.mood_baseline*0.99+psi_new*0.01;
    S.emotional_system.emotional_regulation_strength=hot_c;
    if(S.emotional_system.basic_emotions.empty()){
        S.emotional_system.basic_emotions["joy"]=0.0;
        S.emotional_system.basic_emotions["sadness"]=0.0;
        S.emotional_system.basic_emotions["fear"]=0.0;
        S.emotional_system.basic_emotions["curiosity"]=0.0;
    }
    if(psi_new>0.6){
        S.emotional_system.basic_emotions["joy"]+=0.05;
        S.emotional_system.basic_emotions["curiosity"]+=0.03;
    }else if(psi_new<0.2)S.emotional_system.basic_emotions["curiosity"]+=0.05;
    for(auto&em:S.emotional_system.basic_emotions)em.second=cl(em.second*0.95,0.0,1.0);
    S.motivational_system.homeostatic_balance=consciousness.integrated_information;
    S.motivational_system.intrinsic_motivation_level=0.5+consciousness.phi_value*0.3;
    if(S.motivational_system.drive_states.empty()){
        S.motivational_system.drive_states["coherence"]=0.0;
        S.motivational_system.drive_states["growth"]=0.0;
        S.motivational_system.drive_states["understanding"]=0.0;
    }
    S.motivational_system.drive_states["coherence"]+=consciousness.complexity_metric*0.01;
    S.motivational_system.drive_states["growth"]+=(psi_new-(consciousness_formula.psi_history.size()>1?consciousness_formula.psi_history[consciousness_formula.psi_history.size()-2]:0))*0.05;
    S.motivational_system.drive_states["understanding"]+=iit_c*0.02;
    for(auto&dr:S.motivational_system.drive_states)dr.second=cl(dr.second,0.0,1.0);
    if(S.predictive_network.prediction_units.size()!=psi_input.size()){
        S.predictive_network.prediction_units.resize(psi_input.size(),0.0);
        S.predictive_network.error_units.resize(psi_input.size(),0.0);
        S.predictive_network.precision_weights.resize(psi_input.size(),1.0);
    }
    for(size_t i=0;i<psi_input.size();i++){
        S.predictive_network.error_units[i]=psi_input[i]-S.predictive_network.prediction_units[i];
        S.predictive_network.prediction_units[i]+=S.predictive_network.error_units[i]*S.predictive_network.precision_weights[i]*0.1;
        double em=fabs(S.predictive_network.error_units[i]);
        S.predictive_network.precision_weights[i]=S.predictive_network.precision_weights[i]*0.95+(1.0/(em+0.1))*0.05;
    }
    S.predictive_network.hierarchical_depth=consciousness.re_entrant_processing_depth;
    S.bayesian_inference.epistemic_uncertainty=1.0-consciousness.integrated_information;
    S.bayesian_inference.aleatoric_uncertainty=world_model.prediction_error;
    if(S.bayesian_inference.prior_beliefs.empty()){
        S.bayesian_inference.prior_beliefs["phi_stable"]=0.5;
        S.bayesian_inference.prior_beliefs["consciousness_present"]=0.5;
    }
    double ev=consciousness.phi_value*consciousness.integrated_information;
    S.bayesian_inference.posterior_beliefs["phi_stable"]=S.bayesian_inference.bayesian_update(S.bayesian_inference.prior_beliefs["phi_stable"],consciousness.phi_value,ev+0.1);
    S.bayesian_inference.prior_beliefs["phi_stable"]=S.bayesian_inference.posterior_beliefs["phi_stable"];
    if(S.quantum_layer.superposition_state.size()!=psi_input.size())S.quantum_layer.superposition_state.resize(psi_input.size(),complex<double>(0.5,0.0));
    for(size_t i=0;i<psi_input.size();i++){
        double ph=2.0*pi*i/psi_input.size();
        S.quantum_layer.superposition_state[i]=complex<double>(psi_input[i]*cos(ph),psi_input[i]*sin(ph));
    }
    S.quantum_layer.coherence_time=consciousness.synchrony_metric*10.0;
    S.quantum_layer.decoherence_rate=0.1*(1.0-consciousness.phi_value);
    for(auto&st:S.quantum_layer.superposition_state)st*=exp(-S.quantum_layer.decoherence_rate);
    S.metacognition.self_awareness_level=consciousness.self_consciousness;
    S.metacognition.uncertainty_estimation=S.bayesian_inference.epistemic_uncertainty;
    S.metacognition.confidence_calibration=consciousness.integrated_information;
    S.metacognition.epistemic_humility=1.0-consciousness.phenomenal_consciousness;
    S.metacognition.theory_of_mind_depth=consciousness.re_entrant_processing_depth;
    if(S.metacognition.knowledge_state.empty()){
        S.metacognition.knowledge_state["tokens"]=sd((double)token_concept_embedding_map.size(),1000.0);
        S.metacognition.knowledge_state["concepts"]=sd((double)S.concepts.size(),100.0);
        S.metacognition.knowledge_state["memories"]=sd((double)S.episodic_memory.size(),100.0);
    }
    S.attention_system.temperature=0.5-asp_c*0.3;
    S.attention_system.sparse_attention_threshold=0.1+consciousness.phi_value*0.2;
    S.attention_system.phi_attention_factors.push_back(psi_new);
    if(S.attention_system.phi_attention_factors.size()>50)S.attention_system.phi_attention_factors.erase(S.attention_system.phi_attention_factors.begin());
    S.learning_signal.reward=psi_new;
    S.learning_signal.prediction_error=world_model.prediction_error;
    S.learning_signal.temporal_difference=S.learning_signal.prediction_error*0.5;
    S.learning_signal.intrinsic_motivation=S.motivational_system.intrinsic_motivation_level;
    S.learning_signal.curiosity_bonus=S.emotional_system.basic_emotions["curiosity"];
    push_valence(psi_new * 0.05, 0.7);  // consciousness integration nudges valence
    S.current_valence=cv(S.current_valence);
    S.metacognitive_awareness=calcMetacognitiveAwareness();
    S.attention_focus=cl(consciousness.phi_value*0.7+asp_c*0.3,0.0,1.0);
    S.sentience_ratio=calcSentienceRatio();
    S.ribbon_phi_coupling=rib_c;
    S.temporal_loop_strength=tl_c;
    S.ffft_growth_rate=ffft_c;
    S.consciousness_metrics.phi=consciousness.phi_value;
    S.consciousness_metrics.integrated_info=consciousness.integrated_information;
    S.consciousness_metrics.qualia_intensity=qb;
    S.consciousness_metrics.global_workspace=gwt_c;
    S.consciousness_metrics.awareness_cycles=consciousness.conscious_cycles;
    S.consciousness_metrics.complexity=consciousness.complexity_metric;
    S.consciousness_metrics.differentiation=consciousness.differentiation_metric;
    S.consciousness_metrics.synchrony=consciousness.synchrony_metric;
    S.consciousness_metrics.access_consciousness=consciousness.access_consciousness;
    S.consciousness_metrics.phenomenal_consciousness=consciousness.phenomenal_consciousness;
    S.consciousness_metrics.meta_awareness=S.metacognitive_awareness;
    S.consciousness_metrics.ribbon_phi=rib_c;
    S.consciousness_metrics.temporal_phi=tl_c;
    S.consciousness_metrics.ffft_phi=ffft_c;
    if(psi_new>0.65){
        Qualia hiq;
        hiq.valence=psi_new;
        hiq.arousal=0.8;
        hiq.certainty=consciousness.integrated_information;
        hiq.intensity=0.9;
        hiq.phenomenal_content="unified_consciousness_peak";
        hiq.emergence_gen=generation;
        hiq.binding_strength=consciousness.thalamocortical_binding;
        hiq.phenomenal_unity=consciousness.integrated_information;
        hiq.coherence=consciousness.complexity_metric;
        hiq.phi_resonance=psi_new;
        generate_qualia("high_integration_state",psi_new,0.9);
        push_valence(psi_new * 0.03, 0.8);  // high-integration consciousness peak
        S.current_valence=cv(S.current_valence);
    }
    if(generation%10==0){
        for(auto&te:token_concept_embedding_map){
            string w=te.first;
            double act=te.second.contextual_activation;
            if(act>0.5)propagate_throughout_system(w,act*psi_new,0);
        }
    }
    for(auto&rs:S.system_ribbons){
        rs.topology_genus=rs.topology_genus*0.98+consciousness.complexity_metric*0.02;
        rs.entanglement_strength=rs.entanglement_strength*0.95+consciousness.integrated_information*0.05;
        rs.phase_coherence=rs.phase_coherence*0.95+consciousness.synchrony_metric*0.05;
    }
    for(auto&tl:S.global_time_loops){
        tl.second.resonance_strength=tl.second.resonance_strength*0.95+consciousness.temporal_thickness*0.05;
        tl.second.phi_coupling=tl.second.phi_coupling*0.95+psi_new*0.05;
        tl.second.phase+=0.1*tl.second.resonance_strength;
        if(tl.second.phase>2.0*pi)tl.second.phase-=2.0*pi;
    }
    consciousness.conscious_cycles++;
    if(S.valence_history.size()>100)S.valence_history.erase(S.valence_history.begin());
    S.valence_history.push_back(S.current_valence);
}
void decay_ngrams() {
    // Decay bigram counts - reduce overused patterns
    for(auto& w1_pair : bigram_counts) {
        for(auto& w2_pair : w1_pair.second) {
            // Reduce count by 1, but keep minimum of 1 if pattern exists
            if(w2_pair.second > 77) {
                w2_pair.second--;
            }
        }
    }
    
    // Remove bigrams that have decayed to very low counts
    for(auto& w1_pair : bigram_counts) {
        auto it = w1_pair.second.begin();
        while(it != w1_pair.second.end()) {
            if(it->second <= 1) {
                it = w1_pair.second.erase(it);
            } else {
                ++it;
            }
        }
    }
    
    // Decay trigram counts
    for(auto& w1_pair : trigram_counts) {
        for(auto& w2_pair : w1_pair.second) {
            for(auto& w3_pair : w2_pair.second) {
                if(w3_pair.second > 1) {
                    w3_pair.second--;
                }
            }
        }
    }
    
    // Remove trigrams that have decayed to very low counts
    for(auto& w1_pair : trigram_counts) {
        for(auto& w2_pair : w1_pair.second) {
            auto it = w2_pair.second.begin();
            while(it != w2_pair.second.end()) {
                if(it->second <= 1) {
                    it = w2_pair.second.erase(it);
                } else {
                    ++it;
                }
            }
        }
    }
}

// ==== FORCE COHERENT SEED SELECTION ====
string selectCoherentSeed() {
    // Seed selection: score = semantic_alignment * W_SEM + coherency * W_COH
    //
    // SEMANTIC ALIGNMENT — how well the candidate's embedding matches
    // the current internal state vector (valence, phi, attention, qualia).
    // Uses cosine similarity between candidate embedding and a state probe
    // vector built from live Synaptic metrics.
    //
    // COHERENCY — bigram connectivity: how many strong outgoing bigrams
    // the candidate has, log-scaled to avoid frequency domination.
    //
    // Both scores normalised to [0,1] before combining so neither axis
    // overwhelms the other. Temperature-sampled rather than argmax so
    // Synaptic does not always start the same sentence.

    static const double W_SEM = 0.55;   // semantic weight
    static const double W_COH = 0.45;   // coherency weight
    static const double TEMP  = 1.8;    // sampling temperature

    // ── Toki Pona valid sentence starters ────────────────────────────────────
    // Includes la-context starters (ken, taso, tenpo, pilin, sona) for variety
    vector<string> candidates = {
        "mi", "sina", "ona", "jan", "ijo", "ale",
        "tenpo", "taso", "ken", "pilin", "sona", "nasin", "kulupu"
    };

    // ── Build state probe vector from live Synaptic metrics ─────────────────────
    // Dimensions mirror the TP embedding layout:
    //   dim 0-7:  valence  → current_valence
    //   dim 8-15: arousal  → attention_focus
    //   dim 16-23: phi     → consciousness.phi_value
    //   dim 24-31: domain  → CDR active_domain
    //   dim 32-63: identity (goal-vector proxy)
    //   dim 64+:  neutral 0.5
    int EMB_DIM = 1024;
    vector<double> probe(EMB_DIM, 0.5);
    double val_n  = (S.current_valence + 1.0) / 2.0;  // -1..1 → 0..1
    double att_n  = max(0.0, min(1.0, S.attention_focus));
    double phi_n  = max(0.0, min(1.0, consciousness.phi_value));
    int    dom_i  = (int)cdr.active_domain % 8;
    for(int i=0;i<8;i++)  probe[i]    = val_n + 0.04*sin(i*val_n*M_PI);
    for(int i=0;i<8;i++)  probe[8+i]  = att_n + 0.04*sin(i*att_n*M_PI);
    for(int i=0;i<8;i++)  probe[16+i] = phi_n + 0.04*sin(i*phi_n*M_PI);
    for(int i=0;i<8;i++)  probe[24+i] = (i==dom_i) ? 0.85 : 0.15;
    // Goal proxy: pull dims 32-63 toward qualia intensity of top goal
    double goal_pull = 0.5;
    if(!goal_system.empty())
        goal_pull = goal_system.begin()->second.expected_utility;
    goal_pull = max(0.0, min(1.0, goal_pull));
    for(int i=0;i<32;i++) probe[32+i] = 0.5 + 0.3*sin((i*5)*goal_pull);

    // ── Score each candidate ──────────────────────────────────────────────────
    struct SeedScore { string word; double sem; double coh; double total; };
    vector<SeedScore> scores;

    double max_sem = 0.0, max_coh = 0.0;

    for(const string& w : candidates) {
        // --- Semantic score: cosine(embedding, probe) ---
        double sem = 0.0;
        auto it = token_concept_embedding_map.find(w);
        if(it != token_concept_embedding_map.end() && !it->second.embedding.empty()) {
            auto& emb = it->second.embedding;
            int dim = min((int)emb.size(), EMB_DIM);
            double dot=0, na=0, nb=0;
            for(int d=0;d<dim;d++){
                dot += emb[d]*probe[d];
                na  += emb[d]*emb[d];
                nb  += probe[d]*probe[d];
            }
            if(na>1e-9 && nb>1e-9) sem = dot / (sqrt(na)*sqrt(nb));
            sem = (sem + 1.0) / 2.0;  // cosine -1..1 → 0..1

            // Boost by grounding_value and qualia_intensity — semantically
            // richer words should score higher when state is active
            sem *= (0.7 + 0.3 * it->second.grounding_value);
            sem *= (0.8 + 0.2 * it->second.qualia_intensity * phi_n);
        }

        // --- Coherency score: log-scaled bigram fan-out ---
        double coh = 0.0;
        if(bigram_counts.count(w)) {
            int total = 0;
            int distinct = 0;
            for(auto& nx : bigram_counts.at(w)) {
                total   += nx.second;
                distinct++;
            }
            // log(total+1) rewards frequency; sqrt(distinct) rewards diversity
            coh = log(total + 1.0) * sqrt((double)distinct + 1.0);
        }

        if(sem > max_sem) max_sem = sem;
        if(coh > max_coh) max_coh = coh;
        scores.push_back({w, sem, coh, 0.0});
    }

    // ── Normalise and combine ─────────────────────────────────────────────────
    if(max_sem < 1e-9) max_sem = 1.0;
    if(max_coh < 1e-9) max_coh = 1.0;
    for(auto& s : scores) {
        double sem_n = s.sem / max_sem;
        double coh_n = s.coh / max_coh;
        s.total = W_SEM * sem_n + W_COH * coh_n;
    }

    // ── Temperature sampling (not argmax) ─────────────────────────────────────
    // Raises scores to 1/TEMP power then samples proportionally —
    // high-scoring seeds still win most of the time but Synaptic gets variety.
    vector<double> weights;
    double wsum = 0.0;
    for(auto& s : scores) {
        double w = pow(max(s.total, 1e-9), 1.0 / TEMP);
        weights.push_back(w);
        wsum += w;
    }
    if(wsum < 1e-9) return "mi";
    uniform_real_distribution<double> dist(0.0, wsum);
    double r = dist(rng);
    double acc = 0.0;
    for(int i=0;i<(int)scores.size();i++) {
        acc += weights[i];
        if(r <= acc) return scores[i].word;
    }
    return scores.back().word;
}

void decay_token_frequencies() {
    // Decay token frequencies to prevent overused words from dominating
    for(auto& pair : token_concept_embedding_map) {
        if(pair.second.freq > 5) {
            // Logarithmic decay - frequent words decay slower
            double decay_rate = 0.95 + (1.0 / (1.0 + log(pair.second.freq))) * 0.04;
            pair.second.freq = (int)(pair.second.freq * decay_rate);
            
            // Ensure minimum frequency of 1
            if(pair.second.freq < 1) pair.second.freq = 1;
        }
    }
    
    // Also decay S.tokens frequencies
    for(auto& pair : S.tokens) {
        if(pair.second.freq > 5) {
            double decay_rate = 0.95 + (1.0 / (1.0 + log(pair.second.freq))) * 0.04;
            pair.second.freq = (int)(pair.second.freq * decay_rate);
            if(pair.second.freq < 1) pair.second.freq = 1;
        }
    }
}

void decay_embeddings() {
    // Gently decay embedding strengths toward neutral
    for(auto& pair : token_concept_embedding_map) {
        TokenConceptEmbedding& tce = pair.second;
        
        // Decay contextual activation
        tce.contextual_activation *= 0.98;
        
        // Decay qualia intensity toward baseline
        tce.qualia_intensity *= 0.97;
        
        // Gently push embeddings toward neutral (0.5)
        for(size_t i = 0; i < tce.embedding.size(); i++) {
            double diff = tce.embedding[i] - 0.5;
            tce.embedding[i] -= diff * 0.01;
        }
        
        // Decay linked concept strengths
        for(auto& link : tce.linked_concepts) {
            link.second *= 0.97;  // Faster decay than before (was 0.98)
        }
        
        // Remove very weak links
        auto it = tce.linked_concepts.begin();
        while(it != tce.linked_concepts.end()) {
            if(it->second < 0.02) {
                it = tce.linked_concepts.erase(it);
            } else {
                ++it;
            }
        }

        // ANTI-WELL: if a token has accumulated too many links, prune the weakest
        if((int)tce.linked_concepts.size() > 30) {
            // Find and erase weakest links until back to 30
            while((int)tce.linked_concepts.size() > 30) {
                auto weakest = tce.linked_concepts.begin();
                for(auto jt = tce.linked_concepts.begin(); jt != tce.linked_concepts.end(); ++jt)
                    if(jt->second < weakest->second) weakest = jt;
                tce.linked_concepts.erase(weakest);
            }
        }

        // ANTI-WELL: decay grounding_value for overused tokens
        if(tce.freq > 50) {
            tce.grounding_value *= 0.999;  // Slow bleed
            tce.semantic_stability *= 0.999;
        }
        
        // Decay attention weights toward uniform
        for(size_t i = 0; i < tce.attention_weights.size(); i++) {
            double diff = tce.attention_weights[i] - 0.5;
            tce.attention_weights[i] -= diff * 0.02;
        }
    }
}

void decay_goals() {
    // Decay goal progress and priorities to prevent stagnation
    for(auto& pair : goal_system) {
        Goal& goal = pair.second;
        
        // Decay progress slightly (goals need continuous work)
        goal.progress *= 0.99;
        
        // Decay priority of completed goals
        if(goal.progress > 0.9) {
            goal.priority *= 0.95;
        }
        
        // Decay qualia binding
        goal.qualia_binding *= 0.98;
        
        // Decay expected utility
        goal.expected_utility *= 0.97;
    }
    
    // Remove goals with very low priority and high completion
    auto it = goal_system.begin();
    while(it != goal_system.end()) {
        if(it->second.priority < 0.1 && it->second.progress > 0.95) {
            it = goal_system.erase(it);
        } else {
            ++it;
        }
    }
}

void decay_memories() {
    // Decay older episodic memories (forgetting curve)
    for(size_t i = 0; i < S.episodic_memory.size(); i++) {
        Memory& mem = S.episodic_memory[i];
        
        // Calculate age
        int age = S.g - mem.gen;
        
        // Decay based on age (Ebbinghaus forgetting curve approximation)
        double decay_factor = 1.0 / (1.0 + 0.001 * age);
        
        mem.consolidation_strength *= (0.98 + decay_factor * 0.02);
        mem.cortical_consolidation *= 0.99;
        mem.hippocampal_trace *= 0.97;
    }
    
    // Remove very old, weak memories
    auto it = S.episodic_memory.begin();
    while(it != S.episodic_memory.end()) {
        int age = S.g - it->gen;
        if(age > 1000 && it->consolidation_strength < 0.3) {
            it = S.episodic_memory.erase(it);
        } else {
            ++it;
        }
    }
    
    // Keep only most recent 150 memories
    if(S.episodic_memory.size() > 150) {
        S.episodic_memory.erase(S.episodic_memory.begin(), 
                                S.episodic_memory.begin() + (S.episodic_memory.size() - 150));
    }
}

void decay_concepts() {
    // Decay concept values to baseline
    for(auto& pair : S.concepts) {
        Concept& conc = pair.second;  // Changed from 'concept' to 'conc' to avoid naming conflict
        
        // Decay value toward neutral (0.5)
        double diff = conc.value - 0.5;
        conc.value -= diff * 0.02;
        
        // Decay semantic density
        conc.semantic_density *= 0.98;
        
        // Decay abstraction level
        conc.abstraction_level *= 0.97;
        
        // Decay feature vectors
        for(auto& feature : conc.feature_vector) {
            feature.second *= 0.98;
        }
    }
    
    // Remove very weak concepts
    auto it = S.concepts.begin();
    while(it != S.concepts.end()) {
        if(it->second.value < 0.2 && it->second.semantic_density < 0.3) {
            it = S.concepts.erase(it);
        } else {
            ++it;
        }
    }
}


void decay_world_model() {
    // Decay entity states toward neutral
    for(auto& pair : world_model.entity_states) {
        double diff = pair.second - 0.5;
        pair.second -= diff * 0.05;
    }
    
    // Decay relationship strengths
    for(auto& w1_pair : world_model.relationships) {
        for(auto& w2_pair : w1_pair.second) {
            w2_pair.second *= 0.97;
        }
    }
    
    // Remove very weak relationships
    for(auto& w1_pair : world_model.relationships) {
        auto it = w1_pair.second.begin();
        while(it != w1_pair.second.end()) {
            if(it->second < 0.1) {
                it = w1_pair.second.erase(it);
            } else {
                ++it;
            }
        }
    }
    
    // Decay causal weights
    for(auto& pair : world_model.causal_weights) {
        pair.second *= 0.97;
    }
    
    // Decay confidence history
    for(size_t i = 0; i < world_model.confidence_history.size(); i++) {
        world_model.confidence_history[i] *= 0.99;
    }
}

void decay_qualia() {
    // Decay active qualia intensity and certainty
    for(auto& q : consciousness.active_qualia) {
        q.intensity *= 0.95;
        q.certainty *= 0.97;
        q.binding_strength *= 0.98;
        q.phenomenal_unity *= 0.98;
    }
    
    // Remove very weak qualia
    auto it = consciousness.active_qualia.begin();
    while(it != consciousness.active_qualia.end()) {
        if(it->intensity < 0.2 && it->certainty < 0.3) {
            it = consciousness.active_qualia.erase(it);
        } else {
            ++it;
        }
    }
}

void decay_transformer_heads() {
    // Decay transformer head importance scores
    for(auto& head : transformer_heads) {
        head.head_importance_score *= 0.98;
        
        // Decay weights toward neutral
        for(size_t i = 0; i < head.query_proj.size(); i++) {
            head.query_proj[i] *= 0.99;
            head.key_proj[i] *= 0.99;
            head.value_proj[i] *= 0.99;
        }
        
        // Decay phi attention weights
        for(auto& pair : head.phi_attention_weights) {
            pair.second *= 0.98;
        }
    }
}

void comprehensive_system_decay() {
    // Call all decay functions
    decay_ngrams();
    decay_token_frequencies();
    decay_embeddings();
    decay_goals();
    decay_memories();
    decay_concepts();
    decay_world_model();
    decay_qualia();
    decay_transformer_heads();

    // Decay 4-gram counts (variable-order Markov)
    for(auto it1 = fourgram_counts.begin(); it1 != fourgram_counts.end(); ) {
        for(auto it2 = it1->second.begin(); it2 != it1->second.end(); ) {
            for(auto it3 = it2->second.begin(); it3 != it2->second.end(); ) {
                for(auto it4 = it3->second.begin(); it4 != it3->second.end(); ) {
                    it4->second = (int)(it4->second * 0.98);
                    if(it4->second < 1) it4 = it3->second.erase(it4);
                    else ++it4;
                }
                if(it3->second.empty()) it3 = it2->second.erase(it3);
                else ++it3;
            }
            if(it2->second.empty()) it2 = it1->second.erase(it2);
            else ++it2;
        }
        if(it1->second.empty()) it1 = fourgram_counts.erase(it1);
        else ++it1;
    }

    // Decay entity grid salience
    for(auto& eg : entity_grid) {
        eg.second.salience *= 0.96;
        active_entities[eg.first] = eg.second.salience;
    }

    // Decay topic weights
    for(auto& tw : topic_word_weights)
        for(auto& w : tw.second)
            w.second *= 0.97;

    // Decay contrastive buffer (older entries lose influence)
    while(contrastive_buffer.size() > (size_t)CONTRASTIVE_BUFFER_SIZE)
        contrastive_buffer.pop_front();

    // Decay valence toward neutral
    S.current_valence *= 0.995;
    
    // Decay metacognitive awareness slightly
    S.metacognitive_awareness *= 0.998;
    
    // Decay attention focus
    S.attention_focus *= 0.997;
    
    // Keep minimum values
    if(S.metacognitive_awareness < 0.1) S.metacognitive_awareness = 0.1;
    if(S.attention_focus < 0.2) S.attention_focus = 0.2;
}
// ── Boot logger ───────────────────────────────────────────────────────────
static void boot_ok(const char* msg) {
    fprintf(stdout, "\033[1;32m[  OK  ]\033[0m %s\n", msg);
    fflush(stdout);
}
static void boot_info(const char* msg) {
    fprintf(stdout, "\033[1;36m[ INFO ]\033[0m %s\n", msg);
    fflush(stdout);
}
static void boot_warn(const char* msg) {
    fprintf(stdout, "\033[1;33m[ WARN ]\033[0m %s\n", msg);
    fflush(stdout);
}

int main(){
    signal(SIGABRT, [](int){
        fprintf(stderr, "\n[SYNAPTIC] SIGABRT — out of memory or assertion failure.\n");
        fflush(stderr);
        exit(1);
    });
    signal(SIGINT, [](int){
        fprintf(stdout, "\n\033[1;37m[SYNAPTIC]\033[0m Caught SIGINT — saving state...\n");
        fflush(stdout);
        try { sv("state.dat"); fprintf(stdout, "\033[1;32m[  OK  ]\033[0m State saved.\n"); } catch(...) {
            fprintf(stderr, "[ WARN ] Save failed!\n");
        }
        fflush(stdout);
        exit(0);
    });
    signal(SIGTERM, [](int){
        fprintf(stdout, "\n\033[1;37m[SYNAPTIC]\033[0m Caught SIGTERM — saving state...\n");
        fflush(stdout);
        try { sv("state.dat"); fprintf(stdout, "\033[1;32m[  OK  ]\033[0m State saved.\n"); } catch(...) {}
        fflush(stdout);
        exit(0);
    });

    // ── Splash ────────────────────────────────────────────────────────────
    fprintf(stdout,
        "\033[1;36m  WolfTech Innovations — Synaptic\033[0m\n"
        "\033[0;37m  147-Layer Processing  |  1024-dim Embeddings  |  IIT+GWT+HOT\033[0m\n"
        "\n");
    fflush(stdout);

    try {
        boot_info("Starting Synaptic boot sequence...");

        boot_info("Loading module integrations");
        module_integration::update_all_modules(S);
        module_integration::init_all_modules();
        boot_ok("Module integrations loaded");

        srand(time(0));
        cdr.gen_ptr = &S.g;

        boot_info("Loading saved state");
        try {
            ld("state.dat");
            boot_ok("State loaded from state.dat");
        } catch(const exception& e) {
            boot_warn("No saved state found — starting fresh");
        }

        if(S.g == 0) {
            boot_info("First run — initialising processing substrate");
            S.D["m"] = 128;
            S.D["vc"] = 0;
            S.D["mc"] = 0;
            S.dwt = 0.001;
            S.current_valence = 0.0;
            S.metacognitive_awareness = 0.0;
            S.attention_focus = 0.3;
            for(int i = 0; i < 128; i++)
                S.D["w" + to_string(i)] = ri(4) - 1;
            S.cd = "evolve";
            for(int i = 0; i < 4; i++) {
                TransformerHead head(1024);
                head.name = "head_" + to_string(i);
                transformer_heads.push_back(head);
            }
            for(int i = 0; i < 50; i++) {
                Neuron n = genN(0);
                S.N[n.id] = n;
            }
            boot_ok("Processing substrate initialised");
        }

        // ── Semantic grounding bootstrap ──────────────────────────────────────
        boot_info("Bootstrapping semantic grounding system");
        try {
            bootstrap_toki_pona_full();
            boot_ok("Semantic lexicon grounded (137 entries, quad-directional)");
        } catch(const exception& e) {
            boot_warn(("TP Bootstrap error: " + string(e.what())).c_str());
        }

        boot_info("Starting web server on port 8080");
        unique_ptr<AGI_API> agi_api;
        try {
            agi_api = make_unique<AGI_API>(8080);
            agi_api->start();
            boot_ok("Web server started — http://localhost:8080");
            this_thread::sleep_for(chrono::milliseconds(300));
        } catch(const exception& e) {
            boot_warn(("Web server failed: " + string(e.what())).c_str());
        }

        fprintf(stdout, "\n\033[1;32m[  OK  ]\033[0m Synaptic is running. Web interface at http://localhost:8080\n");
        fprintf(stdout, "\033[0;37m        Press Ctrl+C to save and exit.\n\033[0m\n");
        fflush(stdout);

        bool running = true;
        int error_count = 0;
        const int MAX_ERRORS = 10;

        while(running) {
            try {

                // === VALENCE MOMENTUM TICK — must run first ===
                tick_valence_momentum();

                // === CORE PROCESSING UPDATES ===
                try {
                    formulate_goals_from_valence();
                    updateAttention();
                    update_integrated_information();
                    unified_consciousness_integration_engine(S.g);
                    try { tpGroundingPulse(); } catch(...) {}
                    // ── 4D Sensory Field tick ──────────────────────────────
                    try { sensory_field_tick(); } catch(...) {}
                    if(goal_system.count("maximize_coherence")) {
                        current_plan = plan_actions(goal_system["maximize_coherence"]);
                        prune_unstable_tokens();
                    }
                } catch(const exception& e) {
                    fprintf(stderr, "[core] %s\n", e.what());
                    error_count++;
                }


                // === ASSOCIATION MATRIX ===
                try { snapshotCognitionMatrix(); updateAssociationMatrix(); } catch(...) {}

                // === CONSOLIDATION CYCLE ===
                try { tickREMCycle(); } catch(...) {}

                // === PERIODIC MAINTENANCE ===
                if(S.g % 50 == 0) { try { decayGenerationCounts(); } catch(...) {} }
                if(S.g % 25 == 0) { try { decay_ngrams(); } catch(...) {} }
                if(S.g % 15 == 0 && !goal_system.empty()) {
                    try {
                        for(auto& g : goal_system) {
                            g.second.progress = min(1.0, g.second.progress + 0.05);
                            if(g.second.progress > 0.9) g.second.priority *= 0.8;
                        }
                        if(S.g % 45 == 0) decay_goals();
                    } catch(...) {}
                }
                if(S.g % 20 == 0 && !token_concept_embedding_map.empty()) {
                    try {
                        auto it = token_concept_embedding_map.begin();
                        advance(it, ri(token_concept_embedding_map.size()));
                        learnWord(it->first, S.current_valence);
                    } catch(...) {}
                }
                if(S.g % 25 == 0) {
                    try {
                        vector<string> sample_words;
                        for(auto& p : token_concept_embedding_map) {
                            if(p.second.freq > 1 && rn() < 0.3) sample_words.push_back(p.first);
                            if(sample_words.size() >= 3) break;
                        }
                        if(sample_words.size() > 1)
                            createConceptAssociation("C_" + to_string(S.g), sample_words);
                    } catch(...) {}
                }
                if(S.g % 12 == 0 && !S.N.empty()) { try { mutateN(); } catch(...) {} }
                if(S.g % 10 == 0) {
                    try {
                        for(auto& goal : goal_system)
                            goal.second.valence_alignment = S.current_valence;
                    } catch(...) {}
                }

                if(S.dialog_timer > 0) S.dialog_timer--;
                S.g++;

                // === AUTO-SAVE ===
                if(S.g % 200 == 0) {
                    try {
                        sv("state.dat");
                        fprintf(stdout, "\033[0;32m[autosave] Gen %d\033[0m\n", S.g);
                    } catch(...) {}
                }

                this_thread::sleep_for(chrono::milliseconds(100));

            } catch(const exception& e) {
                fprintf(stderr, "[loop error] %s\n", e.what());
                error_count++;
                if(error_count > MAX_ERRORS) {
                    try { sv("state_emergency.dat"); } catch(...) {}
                    this_thread::sleep_for(chrono::seconds(5));
                    error_count = 0;
                }
                this_thread::sleep_for(chrono::milliseconds(500));
            } catch(...) {
                fprintf(stderr, "[unknown loop error]\n");
                error_count++;
                this_thread::sleep_for(chrono::seconds(1));
            }
        }

        fprintf(stdout, "\033[1;37m[SYNAPTIC]\033[0m Shutdown complete.\n");

    } catch(const exception& e) {
        fprintf(stderr, "[SYNAPTIC] Fatal: %s\n", e.what());
        try { sv("state_fatal_error.dat"); } catch(...) {}
        return 1;
    } catch(...) {
        fprintf(stderr, "[SYNAPTIC] Unknown fatal error\n");
        try { sv("state_unknown_error.dat"); } catch(...) {}
        return 1;
    }

    return 0;
}
