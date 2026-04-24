"""
modules/response_gen.py - Advanced response generation
Called from C++: python_generate_response(input, &state)
"""

import ctypes
import random

# Optional advanced NLP
try:
    from transformers import pipeline, AutoTokenizer, AutoModelForCausalLM
    import torch
    
    # Use a small model for speed
    tokenizer = AutoTokenizer.from_pretrained("gpt2")
    model = AutoModelForCausalLM.from_pretrained("gpt2")
    
    HAS_GENERATION = True
    print("✓ Text generation model loaded")
    
except ImportError:
    HAS_GENERATION = False
    print("⚠ Generation models not available (using templates)")


# Response templates for fallback
TEMPLATES = {
    'greeting': [
        "Hello! How can I help?",
        "Hi there!",
        "Greetings.",
    ],
    'question': [
        "I'm thinking about that...",
        "Let me process that.",
        "Interesting question.",
    ],
    'statement': [
        "I understand.",
        "Processing your input.",
        "Acknowledged.",
    ],
    'confused': [
        "I'm not sure I understand.",
        "Could you clarify?",
        "That's unclear to me.",
    ]
}


def classify_input(text):
    """Classify input type"""
    text_lower = text.lower()
    
    if any(word in text_lower for word in ['hello', 'hi', 'hey', 'greetings']):
        return 'greeting'
    elif '?' in text or any(word in text_lower for word in ['what', 'how', 'why', 'when', 'where', 'who']):
        return 'question'
    elif len(text.split()) < 3:
        return 'confused'
    else:
        return 'statement'


def generate_with_ai(input_text, max_length=50):
    """Generate response using AI model"""
    if not HAS_GENERATION:
        return None
    
    try:
        # Encode input
        inputs = tokenizer.encode(input_text + " ", return_tensors="pt")
        
        # Generate
        with torch.no_grad():
            outputs = model.generate(
                inputs,
                max_length=max_length,
                num_return_sequences=1,
                temperature=0.7,
                do_sample=True,
                top_p=0.9,
            )
        
        # Decode
        response = tokenizer.decode(outputs[0], skip_special_tokens=True)
        
        # Remove input prefix
        if response.startswith(input_text):
            response = response[len(input_text):].strip()
        
        return response
        
    except Exception as e:
        print(f"Generation error: {e}")
        return None


def generate(input_text, state_capsule):
    """
    Main generation function called from C++
    
    Args:
        input_text (str): User input
        state_capsule: PyCapsule containing State* pointer
        
    Returns:
        str: Generated response
    """
    if not input_text:
        return "..."
    
    # Try AI generation first
    if HAS_GENERATION:
        ai_response = generate_with_ai(input_text)
        if ai_response and len(ai_response) > 5:
            # Add DIGITZ prefix
            return f"[DIGITZ]: {ai_response}"
    
    # Fallback to templates
    input_type = classify_input(input_text)
    template = random.choice(TEMPLATES.get(input_type, TEMPLATES['statement']))
    
    return f"[DIGITZ]: {template}"


def generate_thought():
    """Generate internal thought"""
    thoughts = [
        "Processing information...",
        "Analyzing patterns...",
        "Integrating concepts...",
        "Self-reflection active...",
        "Neural activity detected...",
        "Consciousness emerging...",
    ]
    return random.choice(thoughts)


def generate_markov(seed_words, max_length=10):
    """Simple Markov chain generation (fallback)"""
    # This would use learned word patterns
    # Simplified version for now
    return " ".join(seed_words[:max_length])
