// ignore_for_file: non_constant_identifier_names

import 'dart:ffi';
import 'package:ffi/ffi.dart';

final class CSailorEntry extends Struct {
  external Pointer<Utf8> name;
  @Uint8()
  external int isDirectory;
  @Uint64()
  external int size;
  @Uint64()
  external int modifiedTime;
}

final class CSailorListResult extends Struct {
  external Pointer<CSailorEntry> entries;
  @Uint64()
  external int count;
}

typedef SailorProgressNative = Void Function(Uint64 transferred, Uint64 total);
typedef SailorProgressDart = void Function(int transferred, int total);

typedef SailorLogNative = Void Function(Pointer<Utf8> message);
typedef SailorLogDart = void Function(Pointer<Utf8> message);

typedef SailorSetProgressCbNative = Void Function(Pointer<NativeFunction<SailorProgressNative>>);
typedef SailorSetProgressCbDart = void Function(Pointer<NativeFunction<SailorProgressNative>>);

typedef SailorConnectNative = Int32 Function(Pointer<Utf8>, Uint16, Pointer<Utf8>, Pointer<Utf8>);
typedef SailorConnectDart = int Function(Pointer<Utf8>, int, Pointer<Utf8>, Pointer<Utf8>);

typedef SailorDisconnectNative = Int32 Function();
typedef SailorDisconnectDart = int Function();

typedef SailorIsConnectedNative = Int32 Function();
typedef SailorIsConnectedDart = int Function();

typedef SailorListNative = Pointer<CSailorListResult> Function(Pointer<Utf8>);
typedef SailorListDart = Pointer<CSailorListResult> Function(Pointer<Utf8>);

typedef SailorFreeListNative = Void Function(Pointer<CSailorListResult>);
typedef SailorFreeListDart = void Function(Pointer<CSailorListResult>);

typedef SailorPathOpNative = Int32 Function(Pointer<Utf8>);
typedef SailorPathOpDart = int Function(Pointer<Utf8>);

typedef SailorTwoPathOpNative = Int32 Function(Pointer<Utf8>, Pointer<Utf8>);
typedef SailorTwoPathOpDart = int Function(Pointer<Utf8>, Pointer<Utf8>);

class SailorBindings {
  final DynamicLibrary lib;

  late final SailorSetProgressCbDart sailor_set_progress_callback;
  late final SailorConnectDart sailor_connect;
  late final SailorDisconnectDart sailor_disconnect;
  late final SailorIsConnectedDart sailor_is_connected;
  late final SailorListDart sailor_list;
  late final SailorFreeListDart sailor_free_list;
  late final SailorPathOpDart sailor_mkdir;
  late final SailorPathOpDart sailor_rmdir;
  late final SailorTwoPathOpDart sailor_upload;
  late final SailorTwoPathOpDart sailor_download;
  late final SailorPathOpDart sailor_delete;
  late final SailorTwoPathOpDart sailor_rename;

  SailorBindings(this.lib) {
    sailor_set_progress_callback = lib.lookupFunction<SailorSetProgressCbNative, SailorSetProgressCbDart>('sailor_set_progress_callback');
    sailor_connect = lib.lookupFunction<SailorConnectNative, SailorConnectDart>('sailor_connect');
    sailor_disconnect = lib.lookupFunction<SailorDisconnectNative, SailorDisconnectDart>('sailor_disconnect');
    sailor_is_connected = lib.lookupFunction<SailorIsConnectedNative, SailorIsConnectedDart>('sailor_is_connected');
    sailor_list = lib.lookupFunction<SailorListNative, SailorListDart>('sailor_list');
    sailor_free_list = lib.lookupFunction<SailorFreeListNative, SailorFreeListDart>('sailor_free_list');
    sailor_mkdir = lib.lookupFunction<SailorPathOpNative, SailorPathOpDart>('sailor_mkdir');
    sailor_rmdir = lib.lookupFunction<SailorPathOpNative, SailorPathOpDart>('sailor_rmdir');
    sailor_upload = lib.lookupFunction<SailorTwoPathOpNative, SailorTwoPathOpDart>('sailor_upload');
    sailor_download = lib.lookupFunction<SailorTwoPathOpNative, SailorTwoPathOpDart>('sailor_download');
    sailor_delete = lib.lookupFunction<SailorPathOpNative, SailorPathOpDart>('sailor_delete');
    sailor_rename = lib.lookupFunction<SailorTwoPathOpNative, SailorTwoPathOpDart>('sailor_rename');
  }
}
