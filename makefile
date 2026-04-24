# Nexus' BUILD SYSTEM - Complete automation with UI and training

# Build target selection
TARGET_OS ?= linux

# Directories
SRC := src/
OBJ := obj/
OUTPUT_DIR := output/
UI_DIR := 
CORPUS_DIR := corpus/

# Platform-specific settings
ifeq ($(TARGET_OS),windows)
    TARGET := $(OUTPUT_DIR)synaptic.exe
    ZIG := zig
    CXX := zig c++
    CXXFLAGS := -std=c++2b -O3 -target x86_64-windows-gnu -DWINDOWS_BUILD -DNO_CURSES
    LDFLAGS := -lws2_32
    OBJ_EXT := .obj
    PLATFORM_NAME := Windows (Zig cross-compile)
    USE_ZIG := 1
    TRAIN_SCRIPT := download_corpus.bat
else
    TARGET := $(OUTPUT_DIR)synaptic
    CXX := clang++
    CXXFLAGS := -std=c++23 -O3 -pthread -march=native
    LDFLAGS := -lncurses -lm 
    OBJ_EXT := .o
    PLATFORM_NAME := Linux (native)
    USE_ZIG := 0
    TRAIN_SCRIPT := download_corpus.sh
endif

# Source files
MAIN_SRC := $(SRC)main.cpp
MODULE_SRCS := $(SRC)language_module.cpp \
               $(SRC)consciousness_module.cpp \
               $(SRC)metacognition_module.cpp \
               $(SRC)module_integration.cpp \
               $(SRC)web_server.cpp \
               $(SRC)agi_api.cpp \
               $(SRC)enhanced_reasoning.cpp \
               $(SRC)memory_system.cpp \
               $(SRC)consciousness_coherence.cpp \
               $(SRC)goal_planning.cpp \
               $(SRC)grammar_engine.cpp

SRCS := $(MAIN_SRC) $(MODULE_SRCS)
OBJS := $(patsubst $(SRC)%.cpp,$(OBJ)%$(OBJ_EXT),$(SRCS))

HEADERS := $(SRC)uac.h \
           $(SRC)language_module.h \
           $(SRC)consciousness_module.h \
           $(SRC)embodiment_module.h \
           $(SRC)metacognition_module.h \
           $(SRC)module_integration.h \
           $(SRC)web_server.h \
           $(SRC)agi_api.h \
           $(SRC)enhanced_reasoning.h \
           $(SRC)memory_system.h \
           $(SRC)consciousness_coherence.h \
           $(SRC)goal_planning.h \
           $(SRC)grammar_engine.h

# Colors
C_RESET := \033[0m
C_BOLD := \033[1m
C_GREEN := \033[32m
C_BLUE := \033[34m
C_YELLOW := \033[33m
C_CYAN := \033[36m
C_MAGENTA := \033[35m

.DEFAULT_GOAL := all

.PHONY: all clean rebuild run debug release modules help install windows linux \
        check-deps install-zig setup-ui setup-training ui train corpus both-full \
        complete-setup test-ui package

# ═══════════════════════════════════════════════════════════════════════
# MAIN TARGETS
# ═══════════════════════════════════════════════════════════════════════

# Complete one-command setup
complete-setup: install-zig setup-training setup-ui
	@echo "$(C_GREEN)$(C_BOLD)════════════════════════════════════════════════$(C_RESET)"
	@echo "$(C_GREEN)$(C_BOLD)  ✓ NEXUS AGI Complete Setup Finished!$(C_RESET)"
	@echo "$(C_GREEN)$(C_BOLD)════════════════════════════════════════════════$(C_RESET)"
	@echo ""
	@echo "$(C_CYAN)Next steps:$(C_RESET)"
	@echo "  1. Build: $(C_GREEN)make both$(C_RESET)"
	@echo "  2. Train: $(C_GREEN)make train$(C_RESET)"
	@echo "  3. Run UI: $(C_GREEN)make ui$(C_RESET)"
	@echo ""

# Build everything (engines + UI)
both-full: both setup-ui
	@echo "$(C_GREEN)$(C_BOLD)✓ Full build complete!$(C_RESET)"

# Build Nexus engine
all: check-deps $(TARGET)
	@echo "$(C_GREEN)$(C_BOLD)✓ Build complete for $(PLATFORM_NAME)!$(C_RESET)"
	@echo "$(C_BLUE)Executable: $(TARGET)$(C_RESET)"

# ═══════════════════════════════════════════════════════════════════════
# UI SETUP & BUILD
# ═══════════════════════════════════════════════════════════════════════

setup-ui:
	@echo "$(C_CYAN)Setting up .NET UI...$(C_RESET)"
	@mkdir -p $(UI_DIR)
	@if [ ! -f "$(UI_DIR)NexusUI.csproj" ]; then \
		echo "$(C_YELLOW)Creating UI project files...$(C_RESET)"; \
		echo '<Project Sdk="Microsoft.NET.Sdk">' > $(UI_DIR)NexusUI.csproj; \
		echo '  <PropertyGroup>' >> $(UI_DIR)NexusUI.csproj; \
		echo '    <OutputType>WinExe</OutputType>' >> $(UI_DIR)NexusUI.csproj; \
		echo '    <TargetFramework>net8.0</TargetFramework>' >> $(UI_DIR)NexusUI.csproj; \
		echo '    <Nullable>enable</Nullable>' >> $(UI_DIR)NexusUI.csproj; \
		echo '  </PropertyGroup>' >> $(UI_DIR)NexusUI.csproj; \
		echo '  <ItemGroup>' >> $(UI_DIR)NexusUI.csproj; \
		echo '    <PackageReference Include="Avalonia" Version="11.0.10" />' >> $(UI_DIR)NexusUI.csproj; \
		echo '    <PackageReference Include="Avalonia.Desktop" Version="11.0.10" />' >> $(UI_DIR)NexusUI.csproj; \
		echo '    <PackageReference Include="Avalonia.Themes.Fluent" Version="11.0.10" />' >> $(UI_DIR)NexusUI.csproj; \
		echo '  </ItemGroup>' >> $(UI_DIR)NexusUI.csproj; \
		echo '</Project>' >> $(UI_DIR)NexusUI.csproj; \
		$(MAKE) create-ui-source; \
		cd $(UI_DIR) && dotnet restore; \
	else \
		echo "$(C_GREEN)✓ UI project already exists$(C_RESET)"; \
	fi

create-ui-source:
	@echo "Starting . . .""

ui: $(TARGET) setup-ui
	@echo "$(C_GREEN)$(C_BOLD)Starting NEXUS GUI...$(C_RESET)"
	@cd $(UI_DIR) && dotnet run

build-ui: setup-ui
	@echo "$(C_CYAN)Building UI...$(C_RESET)"
	@cd $(UI_DIR) && dotnet build
	@echo "$(C_GREEN)✓ UI built successfully$(C_RESET)"

test-ui: build-ui
	@echo "$(C_CYAN)Testing UI...$(C_RESET)"
	@cd $(UI_DIR) && dotnet build --no-restore

# ═══════════════════════════════════════════════════════════════════════
# TRAINING SCRIPTS SETUP
# ═══════════════════════════════════════════════════════════════════════

setup-training:
	@echo "$(C_CYAN)Setting up training scripts...$(C_RESET)"
	@if [ ! -f "download_corpus.sh" ]; then \
		echo "$(C_YELLOW)Creating download_corpus.sh...$(C_RESET)"; \
		echo '#!/bin/bash' > download_corpus.sh; \
		echo 'echo "Downloading training corpus..."' >> download_corpus.sh; \
		echo 'mkdir -p corpus' >> download_corpus.sh; \
		echo 'curl -L "https://www.gutenberg.org/files/1342/1342-0.txt" -o corpus/book_1.txt 2>/dev/null || true' >> download_corpus.sh; \
		echo 'curl -L "https://www.gutenberg.org/files/2701/2701-0.txt" -o corpus/book_2.txt 2>/dev/null || true' >> download_corpus.sh; \
		echo 'curl -L "https://www.gutenberg.org/files/84/84-0.txt" -o corpus/book_3.txt 2>/dev/null || true' >> download_corpus.sh; \
		echo 'echo "Extracting vocabulary..."' >> download_corpus.sh; \
		echo 'cat corpus/*.txt | tr "[:upper:]" "[:lower:]" | grep -oE "\\b[a-z]{3,15}\\b" | sort | uniq -c | sort -rn | head -n 50000 > corpus/vocabulary.txt 2>/dev/null || true' >> download_corpus.sh; \
		echo 'echo "Training complete! Found $$(wc -l < corpus/vocabulary.txt 2>/dev/null || echo 0) words"' >> download_corpus.sh; \
		chmod +x download_corpus.sh; \
	fi
	@if [ ! -f "download_corpus.bat" ]; then \
		echo "$(C_YELLOW)Creating download_corpus.bat...$(C_RESET)"; \
		echo '@echo off' > download_corpus.bat; \
		echo 'echo Downloading training corpus...' >> download_corpus.bat; \
		echo 'mkdir corpus 2>nul' >> download_corpus.bat; \
		echo 'curl -L "https://www.gutenberg.org/files/1342/1342-0.txt" -o corpus\\book_1.txt 2>nul' >> download_corpus.bat; \
		echo 'curl -L "https://www.gutenberg.org/files/2701/2701-0.txt" -o corpus\\book_2.txt 2>nul' >> download_corpus.bat; \
		echo 'curl -L "https://www.gutenberg.org/files/84/84-0.txt" -o corpus\\book_3.txt 2>nul' >> download_corpus.bat; \
		echo 'echo Training corpus downloaded!' >> download_corpus.bat; \
	fi
	@echo "$(C_GREEN)✓ Training scripts created$(C_RESET)"

train: setup-training $(TARGET)
	@echo "$(C_GREEN)$(C_BOLD)Starting corpus training...$(C_RESET)"
	@./$(TRAIN_SCRIPT)
	@echo "$(C_GREEN)✓ Training complete!$(C_RESET)"

corpus: setup-training
	@echo "$(C_CYAN)Downloading corpus only...$(C_RESET)"
	@./$(TRAIN_SCRIPT)

# ═══════════════════════════════════════════════════════════════════════
# ZIG INSTALLATION
# ═══════════════════════════════════════════════════════════════════════

check-deps:
ifeq ($(TARGET_OS),windows)
	@echo "$(C_CYAN)Checking for Zig compiler...$(C_RESET)"
	@which $(ZIG) > /dev/null 2>&1 || (echo "$(C_YELLOW)ERROR: Zig not found!$(C_RESET)" && \
	 echo "$(C_GREEN)Install with: make install-zig$(C_RESET)" && exit 1)
	@echo "$(C_GREEN)✓ Zig compiler found$(C_RESET)"
endif

install-zig:
	@echo "$(C_CYAN)Installing Zig to ~/.local/bin...$(C_RESET)"
	@mkdir -p ~/.local/bin ~/.local/zig
	@echo "$(C_YELLOW)Downloading Zig 0.11.0...$(C_RESET)"
	@curl -L https://ziglang.org/download/0.11.0/zig-linux-x86_64-0.11.0.tar.xz -o /tmp/zig.tar.xz
	@tar -xf /tmp/zig.tar.xz -C ~/.local/zig --strip-components=1
	@ln -sf ~/.local/zig/zig ~/.local/bin/zig
	@rm /tmp/zig.tar.xz
	@echo "$(C_GREEN)✓ Zig installed!$(C_RESET)"
	@echo "$(C_YELLOW)Add to PATH: export PATH=\$$HOME/.local/bin:\$$PATH$(C_RESET)"

# ═══════════════════════════════════════════════════════════════════════
# BUILD TARGETS
# ═══════════════════════════════════════════════════════════════════════

windows:
	@$(MAKE) TARGET_OS=windows

linux:
	@$(MAKE) TARGET_OS=linux

both: clean
	@echo "$(C_MAGENTA)$(C_BOLD)Building for both platforms...$(C_RESET)"
	@$(MAKE) TARGET_OS=linux
	@$(MAKE) TARGET_OS=windows
	@echo "$(C_GREEN)$(C_BOLD)✓ Both builds complete!$(C_RESET)"
	@ls -lh $(OUTPUT_DIR)

$(TARGET): $(OBJS) | $(OUTPUT_DIR)
	@echo "$(C_YELLOW)Linking $(TARGET)...$(C_RESET)"
ifeq ($(USE_ZIG),1)
	$(ZIG) c++ $(CXXFLAGS) $(OBJS) -o $(TARGET) $(LDFLAGS)
else
	$(CXX) $(CXXFLAGS) $(OBJS) -o $(TARGET) $(LDFLAGS)
endif

$(OBJ)main$(OBJ_EXT): $(SRC)main.cpp $(HEADERS) | $(OBJ)
	@echo "$(C_BLUE)Compiling main.cpp...$(C_RESET)"
ifeq ($(USE_ZIG),1)
	$(ZIG) c++ $(CXXFLAGS) -c $< -o $@
else
	$(CXX) $(CXXFLAGS) -c $< -o $@
endif

$(OBJ)%$(OBJ_EXT): $(SRC)%.cpp $(SRC)%.h | $(OBJ)
	@echo "$(C_BLUE)Compiling $<...$(C_RESET)"
ifeq ($(USE_ZIG),1)
	$(ZIG) c++ $(CXXFLAGS) -c $< -o $@
else
	$(CXX) $(CXXFLAGS) -c $< -o $@
endif

$(OBJ):
	@mkdir -p $(OBJ)

$(OUTPUT_DIR):
	@mkdir -p $(OUTPUT_DIR)

clean:
	@echo "$(C_YELLOW)Cleaning build artifacts...$(C_RESET)"
	@rm -rf $(OBJ) $(OUTPUT_DIR)
	@echo "$(C_GREEN)✓ Clean complete$(C_RESET)"

clean-all: clean
	@echo "$(C_YELLOW)Cleaning everything (including UI and corpus)...$(C_RESET)"
	@rm -rf $(UI_DIR)bin $(UI_DIR)obj $(CORPUS_DIR)
	@echo "$(C_GREEN)✓ Deep clean complete$(C_RESET)"

rebuild: clean all

run: $(TARGET)
ifeq ($(TARGET_OS),linux)
	@echo "$(C_GREEN)$(C_BOLD)Starting Nexus...$(C_RESET)"
	@./$(TARGET)
else
	@echo "$(C_YELLOW)Cannot run Windows executable on Linux!$(C_RESET)"
endif

debug: CXXFLAGS += -g -O0 -DDEBUG
debug: clean all

release: CXXFLAGS += -DNDEBUG
release: clean all

modules: $(filter-out $(OBJ)main$(OBJ_EXT),$(OBJS))
	@echo "$(C_GREEN)✓ Consciousness modules compiled$(C_RESET)"

install: release
ifeq ($(TARGET_OS),linux)
	@echo "$(C_YELLOW)Installing to /usr/local/bin...$(C_RESET)"
	@sudo cp $(TARGET) /usr/local/bin/Nexus
	@sudo chmod +x /usr/local/bin/Nexus
	@echo "$(C_GREEN)✓ Installed!$(C_RESET)"
endif

# ═══════════════════════════════════════════════════════════════════════
# PACKAGING
# ═══════════════════════════════════════════════════════════════════════

package: both build-ui
	@echo "$(C_CYAN)Creating release package...$(C_RESET)"
	@mkdir -p release
	@cp $(OUTPUT_DIR)Nexus release/ 2>/dev/null || true
	@cp $(OUTPUT_DIR)Nexus.exe release/ 2>/dev/null || true
	@cp download_corpus.sh release/ 2>/dev/null || true
	@cp download_corpus.bat release/ 2>/dev/null || true
	@cp README.md release/ 2>/dev/null || true
	@cd $(UI_DIR) && dotnet publish -c Release -o ../release/NexusUI 2>/dev/null || true
	@echo "$(C_GREEN)✓ Package created in release/$(C_RESET)"

# ═══════════════════════════════════════════════════════════════════════
# HELP
# ═══════════════════════════════════════════════════════════════════════

help:
	@echo "$(C_BOLD)════════════════════════════════════════════════════════$(C_RESET)"
	@echo "$(C_BOLD)  NEXUS AGI - Complete Build System$(C_RESET)"
	@echo "$(C_BOLD)════════════════════════════════════════════════════════$(C_RESET)"
	@echo ""
	@echo "$(C_CYAN)$(C_BOLD)🚀 ONE-COMMAND SETUP:$(C_RESET)"
	@echo "  $(C_GREEN)make complete-setup$(C_RESET)    - Install everything (Zig + UI + training)"
	@echo "  $(C_GREEN)make both-full$(C_RESET)         - Build engines + setup UI"
	@echo ""
	@echo "$(C_CYAN)$(C_BOLD)🔨 BUILD COMMANDS:$(C_RESET)"
	@echo "  $(C_GREEN)make$(C_RESET)                   - Build for current platform"
	@echo "  $(C_GREEN)make linux$(C_RESET)             - Build Linux binary"
	@echo "  $(C_GREEN)make windows$(C_RESET)           - Build Windows .exe"
	@echo "  $(C_GREEN)make both$(C_RESET)              - Build both platforms"
	@echo "  $(C_GREEN)make debug$(C_RESET)             - Debug build"
	@echo "  $(C_GREEN)make release$(C_RESET)           - Release build"
	@echo ""
	@echo "$(C_CYAN)$(C_BOLD)🖥️  UI COMMANDS:$(C_RESET)"
	@echo "  $(C_GREEN)make setup-ui$(C_RESET)          - Create .NET UI project"
	@echo "  $(C_GREEN)make build-ui$(C_RESET)          - Build the UI"
	@echo "  $(C_GREEN)make ui$(C_RESET)                - Run the GUI"
	@echo "  $(C_GREEN)make test-ui$(C_RESET)           - Test UI build"
	@echo ""
	@echo "$(C_CYAN)$(C_BOLD)📚 TRAINING COMMANDS:$(C_RESET)"
	@echo "  $(C_GREEN)make setup-training$(C_RESET)    - Create training scripts"
	@echo "  $(C_GREEN)make train$(C_RESET)             - Download corpus + train NEXUS"
	@echo "  $(C_GREEN)make corpus$(C_RESET)            - Download corpus only"
	@echo ""
	@echo "$(C_CYAN)$(C_BOLD)🛠️  UTILITY COMMANDS:$(C_RESET)"
	@echo "  $(C_GREEN)make install-zig$(C_RESET)       - Install Zig cross-compiler"
	@echo "  $(C_GREEN)make run$(C_RESET)               - Build and run (Linux)"
	@echo "  $(C_GREEN)make clean$(C_RESET)             - Remove build artifacts"
	@echo "  $(C_GREEN)make clean-all$(C_RESET)         - Remove everything (UI + corpus)"
	@echo "  $(C_GREEN)make package$(C_RESET)           - Create release package"
	@echo "  $(C_GREEN)make help$(C_RESET)              - Show this help"
	@echo ""
	@echo "$(C_CYAN)$(C_BOLD)📦 TYPICAL WORKFLOWS:$(C_RESET)"
	@echo "  $(C_YELLOW)First-time setup:$(C_RESET)"
	@echo "    make complete-setup"
	@echo "    make both"
	@echo "    make train"
	@echo "    make ui"
	@echo ""
	@echo "  $(C_YELLOW)Quick rebuild:$(C_RESET)"
	@echo "    make rebuild && make ui"
	@echo ""
	@echo "  $(C_YELLOW)Create release:$(C_RESET)"
	@echo "    make package"
	@echo ""
	@echo "$(C_BOLD)════════════════════════════════════════════════════════$(C_RESET)"
