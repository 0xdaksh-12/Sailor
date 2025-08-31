import 'dart:ffi';
import 'dart:io';
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

class SailorFileItem {
  final String name;
  final bool isDirectory;
  final int size;
  final DateTime modifiedTime;

  SailorFileItem({
    required this.name,
    required this.isDirectory,
    required this.size,
    required this.modifiedTime,
  });
}

typedef SailorProgressNative = Void Function(Uint64 transferred, Uint64 total);
typedef SailorLogNative = Void Function(Pointer<Utf8> message);

typedef ConnectNative = Int32 Function(Pointer<Utf8>, Uint16, Pointer<Utf8>, Pointer<Utf8>);
typedef ConnectDart = int Function(Pointer<Utf8>, int, Pointer<Utf8>, Pointer<Utf8>);

typedef StringPathNative = Int32 Function(Pointer<Utf8>);
typedef StringPathDart = int Function(Pointer<Utf8>);

typedef TwoStringNative = Int32 Function(Pointer<Utf8>, Pointer<Utf8>);
typedef TwoStringDart = int Function(Pointer<Utf8>, Pointer<Utf8>);

typedef ListNative = Pointer<CSailorListResult> Function(Pointer<Utf8>);
typedef ListDart = Pointer<CSailorListResult> Function(Pointer<Utf8>);

typedef FreeListNative = Void Function(Pointer<CSailorListResult>);
typedef FreeListDart = void Function(Pointer<CSailorListResult>);

class SailorBindings {
  late final DynamicLibrary _lib;
  late final ConnectDart _connect;
  late final int Function() _disconnect;
  late final int Function() _isConnected;
  late final ListDart _list;
  late final FreeListDart _freeList;
  late final StringPathDart _mkdir;
  late final StringPathDart _rmdir;
  late final StringPathDart _delete;
  late final TwoStringDart _upload;
  late final TwoStringDart _download;
  late final TwoStringDart _rename;

  SailorBindings(String libraryPath) {
    _lib = DynamicLibrary.open(libraryPath);

    _connect = _lib.lookupFunction<ConnectNative, ConnectDart>('sailor_connect');
    _disconnect = _lib.lookupFunction<Int32 Function(), int Function()>('sailor_disconnect');
    _isConnected = _lib.lookupFunction<Int32 Function(), int Function()>('sailor_is_connected');
    _list = _lib.lookupFunction<ListNative, ListDart>('sailor_list');
    _freeList = _lib.lookupFunction<FreeListNative, FreeListDart>('sailor_free_list');
    _mkdir = _lib.lookupFunction<StringPathNative, StringPathDart>('sailor_mkdir');
    _rmdir = _lib.lookupFunction<StringPathNative, StringPathDart>('sailor_rmdir');
    _delete = _lib.lookupFunction<StringPathNative, StringPathDart>('sailor_delete');
    _upload = _lib.lookupFunction<TwoStringNative, TwoStringDart>('sailor_upload');
    _download = _lib.lookupFunction<TwoStringNative, TwoStringDart>('sailor_download');
    _rename = _lib.lookupFunction<TwoStringNative, TwoStringDart>('sailor_rename');
  }

  Future<void> connect(String host, int port, String user, String pass) async {
    final hPtr = host.toNativeUtf8();
    final uPtr = user.toNativeUtf8();
    final pPtr = pass.toNativeUtf8();
    try {
      final res = _connect(hPtr, port, uPtr, pPtr);
      if (res != 0) throw Exception("Connection failed with code: $res");
    } finally {
      calloc.free(hPtr);
      calloc.free(uPtr);
      calloc.free(pPtr);
    }
  }

  Future<List<SailorFileItem>> list(String path) async {
    final pPtr = path.toNativeUtf8();
    try {
      final resPtr = _list(pPtr);
      if (resPtr == nullptr) throw Exception("Failed to list path: $path");

      final items = <SailorFileItem>[];
      final res = resPtr.ref;
      for (int i = 0; i < res.count; ++i) {
        final entry = res.entries[i];
        items.add(SailorFileItem(
          name: entry.name.toDartString(),
          isDirectory: entry.isDirectory == 1,
          size: entry.size,
          modifiedTime: DateTime.fromMillisecondsSinceEpoch(entry.modifiedTime * 1000),
        ));
      }
      _freeList(resPtr);
      return items;
    } finally {
      calloc.free(pPtr);
    }
  }

  Future<void> upload(String localPath, String remoteDir) async {
    final lPtr = localPath.toNativeUtf8();
    final rPtr = remoteDir.toNativeUtf8();
    try {
      final res = _upload(lPtr, rPtr);
      if (res != 0) throw Exception("Upload failed with error code: $res");
    } finally {
      calloc.free(lPtr);
      calloc.free(rPtr);
    }
  }

  Future<void> download(String remotePath, String localPath) async {
    final rPtr = remotePath.toNativeUtf8();
    final lPtr = localPath.toNativeUtf8();
    try {
      final res = _download(rPtr, lPtr);
      if (res != 0) throw Exception("Download failed with error code: $res");
    } finally {
      calloc.free(rPtr);
      calloc.free(lPtr);
    }
  }

  Future<void> delete(String remotePath) async {
    final rPtr = remotePath.toNativeUtf8();
    try {
      final res = _delete(rPtr);
      if (res != 0) throw Exception("Delete failed with error code: $res");
    } finally {
      calloc.free(rPtr);
    }
  }

  Future<void> rename(String source, String destination) async {
    final sPtr = source.toNativeUtf8();
    final dPtr = destination.toNativeUtf8();
    try {
      final res = _rename(sPtr, dPtr);
      if (res != 0) throw Exception("Rename failed with error code: $res");
    } finally {
      calloc.free(sPtr);
      calloc.free(dPtr);
    }
  }

  Future<void> mkdir(String path) async {
    final pPtr = path.toNativeUtf8();
    try {
      final res = _mkdir(pPtr);
      if (res != 0) throw Exception("Mkdir failed with error code: $res");
    } finally {
      calloc.free(pPtr);
    }
  }

  Future<void> rmdir(String path) async {
    final pPtr = path.toNativeUtf8();
    try {
      final res = _rmdir(pPtr);
      if (res != 0) throw Exception("Rmdir failed with error code: $res");
    } finally {
      calloc.free(pPtr);
    }
  }

  void disconnect() => _disconnect();
  bool isConnected() => _isConnected() == 1;
}
