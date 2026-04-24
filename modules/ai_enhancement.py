"""
modules/ai_enhance.py - Text enhancement module
Called from C++: python_enhance_text("hello world")
"""

# Optional AI imports (graceful degradation)
try:
    from transformers import pipeline
    import torch
    
    device = "cuda" if torch.cuda.is_available() else "cpu"
    
    coherence_model = pipeline(
        "text-classification",
        model="textattack/roberta-base-CoLA",
        device=0 if device == "cuda" else -1
    )
    
    sentiment_model = pipeline(
        "sentiment-analysis",
        model="distilbert-base-uncased-finetuned-sst-2-english",
        device=0 if device == "cuda" else -1
    )
    
    HAS_AI = True
    print("✓ AI enhancement models loaded")
    
except ImportError:
    HAS_AI = False
    print("⚠ AI models not available (basic enhancement only)")


def enhance_text(text):
    """
    Main enhancement function called from C++
    
    Args:
        text (str): Input text to enhance
        
    Returns:
        str: Enhanced text
    """
    if not text:
        return text
    
    enhanced = text.strip()
    
    if HAS_AI:
        # Get AI metrics
        try:
            # Coherence check
            coh_result = coherence_model(text)[0]
            coherence = coh_result['score'] if coh_result['label'] == 'acceptable' else 0.3
            
            # Sentiment
            sent_result = sentiment_model(text)[0]
            sentiment = sent_result['score'] if sent_result['label'] == 'POSITIVE' else -sent_result['score']
            
            # Print metrics to stderr (C++ can see this)
            import sys
            print(f"[AI] Coherence: {coherence:.2f}, Sentiment: {sentiment:.2f}", 
                  file=sys.stderr, flush=True)
            
        except Exception as e:
            print(f"[AI] Enhancement error: {e}", file=sys.stderr)
    
    # Basic enhancements
    if enhanced and enhanced[0].islower():
        enhanced = enhanced[0].upper() + enhanced[1:]
    
    if enhanced and enhanced[-1] not in '.!?':
        enhanced += '.'
    
    return enhanced


def get_coherence(text):
    """Get coherence score (0-1)"""
    if not HAS_AI:
        return 0.5
    
    try:
        result = coherence_model(text)[0]
        return result['score'] if result['label'] == 'acceptable' else 0.3
    except:
        return 0.5


def get_sentiment(text):
    """Get sentiment score (-1 to 1)"""
    if not HAS_AI:
        return 0.0
    
    try:
        result = sentiment_model(text)[0]
        return result['score'] if result['label'] == 'POSITIVE' else -result['score']
    except:
        return 0.0


# Module info
__version__ = "1.0.0"
__author__ = "DIGITZ"
