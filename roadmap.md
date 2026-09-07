賛成です。今の段階なら、機能改善を続けるより **「stable baseline を固定して、外に出せる品質へ持っていく」** 方が自然です。

Phase 10 までで、少なくとも semantic capability と routing の主要な論点は十分に評価できています。ここからはロードマップを **改善フェーズ → Release Readiness** に切り替えるのがよいと思います。

おすすめは、次を新しい流れにすることです。

1. **Phase 11 — Release Freeze / Baseline Lock**

   * Phase 7d + 8a + 8b + 8c を正式な release baseline として固定
   * Phase 10 の Candidate 1 は含めない
   * known limitations を整理
   * deferred / rejected experiments を明文化
   * バージョン番号と release branch 方針を決める
   * 以後、新機能は原則入れない

2. **Phase 12 — Release Quality / Compatibility**

   * clean build
   * test suite 全実行
   * Linux / macOS / Windows の確認
   * CLI exit code / stderr / stdout の確認
   * path / quoting / Unicode / repository root detection
   * install / uninstall
   * empty repository、unsupported language、broken source などの graceful failure
   * performance smoke test
   * binary size / startup latency の確認

3. **Phase 13 — CLI / User Experience Polish**

   * `--help`
   * command naming / arguments
   * error messages
   * JSON output stability
   * examples
   * version output
   * diagnostics
   * 「agent が使いやすいか」ではなく「一般ユーザーが理解できるか」に軸を変更
   * breaking CLI change はここで最後にする

4. **Phase 14 — Documentation**

   * README
   * installation
   * quick start
   * supported languages / capabilities
   * command reference
   * examples
   * limitations
   * AI/Coding Agent integration example
   * architecture overview
   * troubleshooting

5. **Phase 15 — Packaging / Distribution**

   * GitHub Release
   * release archive
   * checksums
   * Homebrew / Scoop / winget / cargo/binary distribution等、必要なものだけ
   * CI release workflow
   * version embedding
   * reproducible-ish build の確認
   * LICENSE / NOTICE / third-party licenses

6. **Phase 16 — Release Candidate**

   * `v0.x.0-rc1`
   * clean environment からインストール
   * README だけを見て実際に使えるか確認
   * small / medium / real repository で smoke test
   * Coding Agent との最終 integration smoke test
   * critical bug のみ修正
   * RC中は semantic capability の改善をしない

7. **Phase 17 — v0.x.0 Release**

   * tag
   * binaries
   * changelog
   * release notes
   * known limitations
   * post-release issue template
   * 次フェーズ候補を backlog に戻す

特に重要なのは、ここで **feature freeze を明示すること**です。

```text
Release baseline

Phase 7d Skill
+
Phase 8a unique FQN suffix resolution
+
Phase 8b receiver-type member relationships
+
Phase 8c callable body identity

Phase 10 routing changes:
not included
```

そして、今まで見つかっているものも「直すべきバグ」と「known limitation」に分けた方がいいです。たとえば、

```text
Known / deferred

- callee-side residual declaration/definition gap
- unsupported advanced receiver/type cases
- templates / overloads / inheritance / virtual dispatch
- explicit this-> handling
```

は、release blocker にしない方がよさそうです。

一方、

```text
Potential release blockers

- Windows path quoting
- installation failures
- crashes / panics
- corrupt JSON output
- nondeterministic CLI contract
- incorrect exit status
- repository corruption / unintended edits
```

のようなものは、release readiness の対象です。

ここで評価基準も切り替えた方がいいです。これまでは、

```text
semantic routing
AST calls
tokens
agent trajectory
```

が中心でしたが、release preparation では、

```text
correctness
stability
compatibility
installability
CLI contract
documentation
failure behavior
release reproducibility
```

を上位に持ってきます。

ロードマップ表にすると、かなりきれいです。

```text
P0–P10
    Core capability / agent optimization
    ✓ COMPLETE

P11 Release Freeze / Baseline Lock
    → NEXT

P12 Release Quality / Compatibility

P13 CLI / UX Polish

P14 Documentation

P15 Packaging / Distribution

P16 Release Candidate

P17 Initial Public Release
```

個人的には、次は **Phase 11 をかなり短いフェーズにする**のがいいと思います。ここで「何をリリースするのか」を固定してしまえば、その後の Coding Agent が勝手に semantic engine を改善し始めるのを防げます。

必要なら次に、そのまま Coding Agent に渡せる **Phase 11 — Release Freeze / Baseline Lock の英語指示書**を作れます。
