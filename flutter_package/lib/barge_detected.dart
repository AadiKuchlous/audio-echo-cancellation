import 'dart:async';
import 'dart:ffi';
import 'dart:io';
import 'dart:isolate';

import 'package:ffi/ffi.dart';

import 'bindings_generated.dart';

const String _libName = 'barge_detected';

/// The dynamic library in which the symbols for [AudioEchoCancellationBindings] can be found.
final DynamicLibrary _dylib = () {
  if (Platform.isMacOS || Platform.isIOS) {
    return DynamicLibrary.open('$_libName.framework/$_libName');
  }
  if (Platform.isAndroid || Platform.isLinux) {
    return DynamicLibrary.open('lib_$_libName.so');
  }
  if (Platform.isWindows) {
    return DynamicLibrary.open('$_libName.dll');
  }
  throw UnsupportedError('Unknown platform: ${Platform.operatingSystem}');
}();

/// The bindings to the native functions in [_dylib].
final AudioEchoCancellationBindings _bindings = AudioEchoCancellationBindings(_dylib);

bool bargeDetected(int buffer_size, List<int> ai_voice, List<int> microphone_signal) {

  Pointer<Int16> ai_voice_pointer = malloc.allocate<Int16>(sizeOf<Int16>() * ai_voice.length);
  Pointer<Int16> microphone_signal_pointer = malloc.allocate<Int16>(sizeOf<Int16>() * microphone_signal.length);
  for (int i = 0; i < buffer_size; i++) {
    ai_voice_pointer.elementAt(i).value = ai_voice[i];
    microphone_signal_pointer.elementAt(i).value = microphone_signal[i];
  }
  return _bindings.bargeDetected(buffer_size, ai_voice_pointer, microphone_signal_pointer);
}
