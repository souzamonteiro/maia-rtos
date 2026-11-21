"""Validate the SDMMC runtime hardware-runner parsing and restoration inputs."""
import importlib.util
from pathlib import Path

root = Path(__file__).resolve().parents[2]
spec = importlib.util.spec_from_file_location(
    "runtime_runner", root / "tests/hardware/run_sdmmc_runtime_probe.py")
runner = importlib.util.module_from_spec(spec)
spec.loader.exec_module(runner)


def records(data):
    lines = []
    for block in range(2):
        for offset in range(0, 512, 16):
            chunk = data[block * 512 + offset:block * 512 + offset + 16]
            lines.append(
                f"SDMMC_RUNTIME_DATA block={block:08x} offset={offset:08x} "
                f"data={chunk.hex()}")
    return "\n".join(lines)


original = bytes((index * 11 + 7) & 0xFF for index in range(1024))
pattern = runner.test_pattern(69318, original)
assert len(pattern) == 1024 and pattern != original

status = ("SDMMC_RUNTIME_OP mode=read result=00000000 completed=00000002 "
          "lba=00010ec6 matches=00000001 mismatch=00000400\n")
capture = (status + records(original) +
           "\nMAIA_NATIVE_SDMMC_RUNTIME_PASS\n").encode()
ok, detail, data = runner.evaluate_operation(capture, "R", 69318)
assert ok and detail == "PASS" and data == original

compare = ("SDMMC_RUNTIME_OP mode=compare result=00000000 completed=00000002 "
           "lba=00010ec6 matches=00000001 mismatch=00000400\n"
           "MAIA_NATIVE_SDMMC_RUNTIME_PASS\n").encode()
assert runner.evaluate_operation(compare, "C", 69318, original)[0]
assert not runner.evaluate_operation(compare, "C", 69319, original)[0]
assert not runner.evaluate_operation(compare.replace(
    b"completed=00000002", b"completed=00000001"), "C", 69318, original)[0]
assert not runner.evaluate_operation(compare.replace(
    b"MAIA_NATIVE_SDMMC_RUNTIME_PASS", b"MAIA_NATIVE_SDMMC_RUNTIME_FAIL"), "C", 69318, original)[0]

all_records = records(original)
missing = (status + all_records[:all_records.find("SDMMC_RUNTIME_DATA block=00000001")] +
           "\nMAIA_NATIVE_SDMMC_RUNTIME_PASS\n").encode()
assert not runner.evaluate_operation(missing, "R", 69318)[0]
duplicate = capture + records(original).splitlines()[0].encode() + b"\n"
assert not runner.evaluate_operation(duplicate, "R", 69318)[0]
assert runner.parse_blocks(capture) == original
print("PASS: runtime SDMMC records require exact range, completion, mode, and both sectors")
