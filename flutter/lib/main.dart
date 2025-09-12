import 'dart:io';
import 'package:flutter/material.dart';
import 'package:flutter_riverpod/flutter_riverpod.dart';
import 'package:window_manager/window_manager.dart';
import 'features/connect/connect_screen.dart';
import 'features/providers.dart';

void main() async {
  WidgetsFlutterBinding.ensureInitialized();

  if (Platform.isLinux || Platform.isWindows || Platform.isMacOS) {
    try {
      await windowManager.ensureInitialized();
      const windowOptions = WindowOptions(
        size: Size(1400, 900),
        minimumSize: Size(1280, 720),
        center: true,
        backgroundColor: Colors.transparent,
        skipTaskbar: false,
        title: 'Sailor File Transfer',
      );
      await windowManager.waitUntilReadyToShow(windowOptions, () async {
        await windowManager.show();
        await windowManager.focus();
      });
    } catch (_) {
      // Graceful fallback if running in environments without native window manager
    }
  }

  runApp(const ProviderScope(child: SailorApp()));
}

class SailorApp extends ConsumerWidget {
  const SailorApp({super.key});

  @override
  Widget build(BuildContext context, WidgetRef ref) {
    final settings = ref.watch(settingsProvider);

    return MaterialApp(
      title: 'Sailor',
      debugShowCheckedModeBanner: false,
      themeMode: settings.themeMode,
      theme: ThemeData(
        useMaterial3: true,
        brightness: Brightness.light,
        colorSchemeSeed: const Color(0xFF0284C7), // Sky 600
        scaffoldBackgroundColor: const Color(0xFFF8FAFC),
        cardTheme: const CardThemeData(
          color: Colors.white,
          elevation: 1,
        ),
      ),
      darkTheme: ThemeData(
        useMaterial3: true,
        brightness: Brightness.dark,
        colorScheme: const ColorScheme.dark(
          primary: Color(0xFF38BDF8), // Sky 400
          secondary: Color(0xFF818CF8), // Indigo 400
          surface: Color(0xFF0F172A), // Slate 900
          surfaceContainerHighest: Color(0xFF1E293B), // Slate 800
          onSurface: Color(0xFFF1F5F9),
          error: Color(0xFFF87171),
        ),
        scaffoldBackgroundColor: const Color(0xFF0B1120),
        cardTheme: const CardThemeData(
          color: Color(0xFF0F172A),
          elevation: 2,
        ),
        dividerColor: const Color(0xFF334155),
      ),
      home: const ConnectScreen(),
    );
  }
}
