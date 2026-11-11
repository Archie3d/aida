#!/usr/bin/env python3
"""Strict, reviewed grading of the pinned B-test selection (not certification)."""

import argparse
from collections import Counter
import importlib.util
import json
from pathlib import Path
import re
import sys
import tempfile
from concurrent.futures import ThreadPoolExecutor

sys.dont_write_bytecode = True
kRoot = Path(__file__).resolve().parent
spec = importlib.util.spec_from_file_location("baseline", kRoot.parent / "baseline/run.py")
baseline = importlib.util.module_from_spec(spec)
spec.loader.exec_module(baseline)
kNote = re.compile(r"^(.*\.(?:a|ada|ads|adb)):\d+:\d+: note: .+$", re.I)
kDiagnostic = re.compile(r"^(.*\.(?:a|ada|ads|adb)):(\d+):(\d+): error: (.*)$", re.I)


def markers(text):
    """Read marker lines only; reviewed spans handle multiline constructs.

    Upstream brace annotations are deliberately not interpreted as our own
    format. All accepted locations are explicit in policies.json.
    """
    result = []
    for number, line in enumerate(text.splitlines(), 1):
        comment = line.split("--", 1)[1] if "--" in line else ""
        match = re.match(r"\s*(?:(OPTIONAL)\s+)?ERROR:", comment, re.I)
        if match:
            result.append({"line": number, "optional": bool(match[1])})
    return result


def expectations(case, policy):
    sites = []
    for source in case["sources"]:
        for marker in markers((baseline.kRoot / "upstream" / source).read_text()):
            key = f"{source}:{marker['line']}"
            rule = policy.get("sites", {}).get(key, {})
            sites.append(dict(marker, source=source,
                              lines=rule.get("lines", [marker["line"]]),
                              groups=rule.get("groups", policy["groups"])))
    keys = {f"{site['source']}:{site['line']}" for site in sites}
    if set(policy.get("sites", {})) - keys:
        raise ValueError(f"policy references an unknown marker: {case['id']}")
    for site in sites:
        if not site["lines"] or any(not isinstance(line, int) or line < 1 for line in site["lines"]):
            raise ValueError("invalid expected location")
        for pattern in site["groups"]:
            re.compile(pattern)
    if not sites:
        raise ValueError(f"no error markers: {case['id']}")
    return sites


def grade(sites, policy, result, legal):
    critical = baseline.abnormal(result) or baseline.abnormal(legal)
    if critical:
        return {"outcome": critical}
    missing, unexpected, matched = [], [], set()
    for line in result["stderr"].splitlines():
        # A related-location note cannot satisfy an error marker. Legal
        # controls still require completely empty compiler output below.
        if kNote.fullmatch(line):
            continue
        diagnostic = kDiagnostic.fullmatch(line)
        if not diagnostic:
            if line.strip():
                unexpected.append(line)
            continue
        path, number, column, message = diagnostic.groups()
        number = int(number)
        # Unsupported prerequisites and parser cascades cannot satisfy a site.
        if re.search(r"unsupported|not (?:yet )?supported|not implemented", message, re.I):
            unexpected.append(line)
            continue
        candidates = [(i, site) for i, site in enumerate(sites)
                      if path.replace('\\', '/').endswith('/' + site["source"])
                      and number in site["lines"]]
        accepted = False
        for index, site in candidates:
            if any(re.fullmatch(pattern, message) for pattern in site["groups"]):
                matched.add(index)
                accepted = True
                break
        # Recovery allowances never discharge a required error site.
        if not accepted:
            accepted = any(path.replace('\\', '/').endswith('/' + rule["source"])
                           and number in rule["lines"]
                           and re.fullmatch(rule["message"], message)
                           for rule in policy.get("recovery", []))
        if not accepted:
            unexpected.append(line)
    for index, site in enumerate(sites):
        if index not in matched and not site["optional"]:
            missing.append(f"{site['source']}:{site['line']}")
    passed = (result["exit"] == 1 and not result["stdout"] and not missing
              and not unexpected and legal["exit"] == 0
              and not legal["stderr"] and not legal["stdout"])
    return {"outcome": "PASS" if passed else "FAIL", "missing": missing,
            "unexpected": unexpected, "compile_exit": result["exit"],
            "legal_exit": legal["exit"], "legal_stderr": legal["stderr"],
            "legal_stdout": legal["stdout"]}


def observe(case, policy, args):
    work = args.work / case["id"]
    work.mkdir(parents=True, exist_ok=True)
    sites = expectations(case, policy)
    with tempfile.TemporaryDirectory(prefix="attempt-", dir=work) as temporary:
        directory = Path(temporary)
        def compileSources(sources, name):
            result = baseline.runCommand([str(args.adac), "-o", str(directory / (name + '.ssa')),
                                          *map(str, sources)], args.timeout)
            result["stderr"] = baseline.normalize(result["stderr"], directory)
            return result
        sources = [baseline.kRoot / 'upstream' / name for name in case["sources"]]
        # Support is compiled normally; errors in helpers are unexpected.
        helpers = [kRoot.parent / 'support' / ('report.' + ext) for ext in ['ads', 'adb']]
        result = compileSources(helpers + sources, 'test')
        legal = compileSources([kRoot / policy["legal"]], 'legal')
        value = grade(sites, policy, result, legal)
        (work / 'compile.log').write_text(result["stdout"] + result["stderr"])
        (work / 'grade.json').write_text(json.dumps(value, indent=2) + '\n')
        return case["id"], value


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--adac', type=Path, required=True)
    parser.add_argument('--work', type=Path, required=True)
    parser.add_argument('--case', action='append', default=[])
    parser.add_argument('--jobs', type=int, default=2)
    parser.add_argument('--timeout', type=float, default=30)
    modes = parser.add_mutually_exclusive_group()
    modes.add_argument('--require-pass', action='store_true')
    modes.add_argument('--check', action='store_true', help='check reviewed grades; FAIL expectations are not passes')
    modes.add_argument('--observe', action='store_true', help='report failures without requiring conformance')
    args = parser.parse_args()
    args.adac = args.adac.resolve(strict=True)
    args.work = args.work.resolve()
    if args.jobs < 1 or args.timeout <= 0:
        parser.error('positive jobs and timeout required')
    manifest = baseline.readJson(baseline.kRoot / 'manifest.json')
    baseline.validate(manifest)
    policies = baseline.readJson(kRoot / 'policies.json')
    cases = [case for case in manifest['cases'] if case['class'] == 'B']
    if set(policies) != {case['id'] for case in cases}:
        parser.error('every B-test requires a reviewed policy')
    if set(args.case) - set(policies):
        parser.error('unknown B-test')
    cases = [case for case in cases if not args.case or case['id'] in args.case]
    with ThreadPoolExecutor(max_workers=args.jobs) as pool:
        results = dict(pool.map(lambda case: observe(case, policies[case['id']], args), cases))
    args.work.mkdir(parents=True, exist_ok=True)
    (args.work / 'results.json').write_text(json.dumps(results, indent=2) + '\n')
    print(dict(Counter(value['outcome'] for value in results.values())))
    for name, value in results.items():
        print(name + ': ' + value['outcome'])
    drift = False
    if args.check:
        expected = baseline.readJson(kRoot / 'expected.json')
        for name, value in results.items():
            if value != expected[name]:
                drift = True
                print('DRIFT ' + name + '\n' + baseline.observationDiff(expected[name], value))
    return int(drift or any(value['outcome'] in baseline.kCritical or
                   (not args.observe and not args.check and value['outcome'] != 'PASS') for value in results.values()))


if __name__ == '__main__':
    try:
        sys.exit(main())
    except (ValueError, OSError, KeyError) as error:
        print(f'grading error: {error}', file=sys.stderr)
        sys.exit(2)
