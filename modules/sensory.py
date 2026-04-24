"""
modules/sensory.py - Sensory input processing (vision + audio)
Called from C++: python_process_sensory(&state)
"""

import ctypes
import numpy as np

# Optional imports
try:
    import cv2
    HAS_VISION = True
except ImportError:
    HAS_VISION = False
    print("⚠ OpenCV not available (vision disabled)")

try:
    import pyaudio
    HAS_AUDIO = True
except ImportError:
    HAS_AUDIO = False
    print("⚠ PyAudio not available (audio disabled)")

try:
    import whisper
    HAS_WHISPER = True
except ImportError:
    HAS_WHISPER = False


# Camera capture
_camera = None
_audio_stream = None

def init_camera(camera_id=0):
    """Initialize camera"""
    global _camera
    
    if not HAS_VISION:
        return False
    
    try:
        _camera = cv2.VideoCapture(camera_id)
        if _camera.isOpened():
            print(f"✓ Camera {camera_id} opened")
            return True
    except Exception as e:
        print(f"Camera error: {e}")
    
    return False


def capture_frame():
    """Capture and process camera frame"""
    if not _camera or not _camera.isOpened():
        return None
    
    ret, frame = _camera.read()
    if not ret:
        return None
    
    # Simple feature extraction: average color values
    features = {
        'brightness': np.mean(frame),
        'r_mean': np.mean(frame[:,:,2]),
        'g_mean': np.mean(frame[:,:,1]),
        'b_mean': np.mean(frame[:,:,0]),
        'contrast': np.std(frame)
    }
    
    return features


def init_audio(rate=16000):
    """Initialize microphone"""
    global _audio_stream
    
    if not HAS_AUDIO:
        return False
    
    try:
        audio = pyaudio.PyAudio()
        _audio_stream = audio.open(
            format=pyaudio.paInt16,
            channels=1,
            rate=rate,
            input=True,
            frames_per_buffer=1024
        )
        print("✓ Microphone opened")
        return True
    except Exception as e:
        print(f"Audio error: {e}")
    
    return False


def capture_audio(duration=1.0):
    """Capture audio snippet"""
    if not _audio_stream:
        return None
    
    try:
        frames = []
        for _ in range(int(16000 / 1024 * duration)):
            data = _audio_stream.read(1024, exception_on_overflow=False)
            frames.append(data)
        
        audio_data = np.frombuffer(b''.join(frames), dtype=np.int16)
        
        # Simple features
        features = {
            'volume': float(np.abs(audio_data).mean()),
            'energy': float(np.sum(audio_data ** 2)),
        }
        
        return features
    except Exception as e:
        print(f"Audio capture error: {e}")
        return None


def process(state_capsule):
    """
    Main processing function called from C++
    
    Args:
        state_capsule: PyCapsule containing State* pointer
    """
    # Get State pointer from capsule
    state_ptr = ctypes.pythonapi.PyCapsule_GetPointer(
        ctypes.py_object(state_capsule), 
        b"State"
    )
    
    # Initialize sensors on first call
    if _camera is None:
        init_camera()
    if _audio_stream is None:
        init_audio()
    
    # Capture sensory data
    visual = capture_frame()
    audio = capture_audio(0.5)
    
    if visual:
        # Update state with visual features
        # Note: You'd need to define how to access State members
        # This is a simplified example
        print(f"[Vision] Brightness: {visual['brightness']:.1f}")
    
    if audio:
        print(f"[Audio] Volume: {audio['volume']:.1f}")


def cleanup():
    """Cleanup resources"""
    global _camera, _audio_stream
    
    if _camera:
        _camera.release()
        _camera = None
    
    if _audio_stream:
        _audio_stream.stop_stream()
        _audio_stream.close()
        _audio_stream = None


# Auto-cleanup on module unload
import atexit
atexit.register(cleanup)
