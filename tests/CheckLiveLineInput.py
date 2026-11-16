"""Check newline completion while the terminal/pipe producer stays open."""

import argparse
import os
from pathlib import Path
import queue
import subprocess
import threading


def check(program, terminal):
    master = slave = None
    if terminal:
        import pty
        master, slave = pty.openpty()
    process = subprocess.Popen([str(program)], stdin=slave if terminal else subprocess.PIPE,
                               stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    if slave is not None:
        os.close(slave)
    lines = queue.Queue()

    def collect():
        for line in iter(process.stdout.readline, b''):
            lines.put(line.rstrip(b'\r\n'))
        lines.put(None)

    reader = threading.Thread(target=collect, daemon=True)
    reader.start()

    def expect(expected):
        try:
            actual = lines.get(timeout=5)
        except queue.Empty:
            raise AssertionError(f'blocked waiting for {expected!r}, terminal={terminal}')
        assert actual == expected, (actual, expected, terminal)

    try:
        expect(b'PROGRAM READY')
        for data, answer in [(b'hello\n', b'hello'), (b'\n', b''),
                             (b'\fworld\n', b'world'), (b'wide\n', b'wide'),
                             (b'wider\n', b'wider'), (b'ignored\n', b'skipped'),
                             (b'abcdef\n', b'abcd')]:
            if terminal:
                assert os.write(master, data) == len(data)
            else:
                process.stdin.write(data)
                process.stdin.flush()
            expect(b'ANSWER ' + answer)
        expect(b'ANSWER ef')
        if terminal:
            assert os.write(master, b'1234\n') == 5
        else:
            process.stdin.write(b'1234\n')
            process.stdin.flush()
        expect(b'ANSWER 1234')
        expect(b'ANSWER ')
        for answer in [b'immediate', b'stream']:
            if terminal:
                assert os.write(master, b'x\frest\n') == 7
            else:
                process.stdin.write(b'x\frest\n')
                process.stdin.flush()
            expect(b'ANSWER ' + answer)
        assert process.wait(timeout=5) == 0
        assert process.stderr.read() == b''
        reader.join(timeout=5)
        assert lines.get(timeout=5) is None
    finally:
        if process.poll() is None:
            process.kill()
        process.wait()
        reader.join(timeout=5)
        if master is not None:
            os.close(master)
        if process.stdin is not None:
            process.stdin.close()
        process.stdout.close()
        process.stderr.close()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--ada', required=True)
    parser.add_argument('--source', required=True)
    parser.add_argument('--work', type=Path, required=True)
    args = parser.parse_args()
    args.work.mkdir(parents=True, exist_ok=True)
    program = args.work / ('getlinelive.exe' if os.name == 'nt' else 'getlinelive')
    subprocess.run([args.ada, '-o', str(program), args.source], check=True, timeout=60)
    check(program, False)
    if os.name == 'posix':
        check(program, True)


if __name__ == '__main__':
    main()
