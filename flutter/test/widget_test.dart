import 'package:flutter/material.dart';
import 'package:flutter_riverpod/flutter_riverpod.dart';
import 'package:flutter_test/flutter_test.dart';
import 'package:sailor_desktop/features/connect/connect_screen.dart';

void main() {
  testWidgets('Connect screen renders all input fields and connect button', (WidgetTester tester) async {
    await tester.pumpWidget(
      const ProviderScope(
        child: MaterialApp(
          home: ConnectScreen(),
        ),
      ),
    );

    expect(find.text('Sailor File Transfer'), findsOneWidget);
    expect(find.text('Host / IP'), findsOneWidget);
    expect(find.text('Port'), findsOneWidget);
    expect(find.text('Username'), findsOneWidget);
    expect(find.text('Password'), findsOneWidget);
    expect(find.text('Connect to Server'), findsOneWidget);
    expect(find.byIcon(Icons.shield_outlined), findsOneWidget);
  });
}
