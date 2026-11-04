Native regression for batched STRING_DIFF common-fragment filling.

After an existing Unix Makefiles seekdb build, run from the repository root:

```sh
python3 unittest/run_native_regression.py --build-dir build_release \
  --source unittest/storage/blocksstable/test_string_diff_common_batch.cpp --sanitize
```

No extra packages or test framework are downloaded. The runner reuses the build's
compiler settings and dependency libraries and records its exact commands.
UBSan instruments the test and inline helpers; product libraries keep their
existing configuration.

The test compares 8192 deterministic layouts against the original scalar common
fragment copier. It covers empty/zero-length descriptors, one-byte and 127-byte
runs, the 128-byte allocation boundary, raw and hex metadata layouts, varied row
strides, batch sizes through 1024, differential bytes, padding and outer guards.
Every buffer byte must match. Native SQL acceptance and complete official
benchmarks are separate gates; this test's runtime is not a performance score.
