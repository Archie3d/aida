"""Compile and link against checked interfaces after removing library sources."""

import argparse
import os
from pathlib import Path
import shutil
import struct
import subprocess


def run(command, environment, expected=None, diagnostic=None):
    result = subprocess.run(list(map(str, command)), env=environment, capture_output=True,
                            text=True, timeout=60)
    if diagnostic is not None:
        if result.returncode != 1 or diagnostic not in result.stderr:
            raise RuntimeError(f'expected {diagnostic}: {command}\n{result.returncode}\n{result.stdout}{result.stderr}')
    elif result.returncode != 0 or (expected is not None and (result.stdout != expected or result.stderr)):
        raise RuntimeError(f'command failed: {command}\n{result.returncode}\n{result.stdout}{result.stderr}')
    return result


def write(path, text):
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(text + '\n')


def envelope(data):
    value = 14695981039346656037
    for byte in data:
        value = ((value ^ byte) * 1099511628211) & ((1 << 64) - 1)
    return b'AIDA semantic interface 1\n' + f'{value:016x}\n'.encode() + data


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--ada', type=Path, required=True)
    parser.add_argument('--adac', type=Path, required=True)
    parser.add_argument('--work', type=Path, required=True)
    parser.add_argument('--exe-suffix', default='')
    args = parser.parse_args()
    args.ada, args.adac = args.ada.resolve(), args.adac.resolve()
    work = args.work.resolve()
    if work.exists():
        shutil.rmtree(work)
    work.mkdir(parents=True)
    library, app, objects = work / 'library sources', work / 'app', work / 'producer'
    environment = os.environ.copy()
    for key in ['ADA_LIBRARY', 'ADA_INCLUDE_PATH', 'ADAC', 'QBE', 'ADA_RUNTIME']:
        environment.pop(key, None)
    environment['ADA_INCLUDE_PATH'] = str(library)
    model = '''package Model is
    Seed : constant Integer := 5;
    type Item is private;
    function Make (N : Integer := Seed) return Item;
    function Value (X : Item) return Integer;
    function Value (X : Integer) return Integer;
    type Tagged_Item is tagged record N : Integer; end record;
    function Next (X : Tagged_Item) return Integer;
    Failure : exception;
private
    type Item is record N : Integer; end record;
end Model;'''
    model_body = '''package body Model is
    function Make (N : Integer := Seed) return Item is begin return (N => N); end Make;
    function Value (X : Item) return Integer is begin return X.N; end Value;
    function Value (X : Integer) return Integer is begin return X + 1; end Value;
    function Next (X : Tagged_Item) return Integer is begin return X.N + 1; end Next;
end Model;'''
    write(library / 'model.ads', model)
    write(library / 'model.adb', model_body)
    write(library / 'model/child.ads', 'package Model.Child is function Extra return Integer; end Model.Child;')
    write(library / 'model/child.adb', '''package body Model.Child is
    function Extra return Integer is begin return Value (Make (3)); end Extra;
end Model.Child;''')
    write(library / 'tools.ads', '''generic type Item is private;
package Tools is function Identity (X : Item) return Item; end Tools;''')
    write(library / 'tools.adb', '''package body Tools is
    function Identity (X : Item) return Item is separate; end Tools;''')
    write(library / 'tools/identity.adb', '''separate (Tools)
function Identity (X : Item) return Item is begin return X; end Identity;''')
    write(library / 'helper.adb', 'procedure Helper is begin null; end Helper;')
    write(library / 'alias.ads', 'with Model; package Alias renames Model;')
    bundle = library / 'custom_bundle.ada'
    write(bundle, '''package Extras is N : constant Integer := 7; end Extras;
package Other_Extras is N : constant Integer := 1; end Other_Extras;''')
    write(library / 'consumer.ads', 'with Extras; package Consumer is N : constant Integer := Extras.N; end Consumer;')
    split_api = library / 'split_api.ada'
    split_body = library / 'split_impl.ada'
    write(split_api, 'package Split is function Value return Integer; end Split;')
    write(split_body, 'package body Split is function Value return Integer is begin return 11; end Value; end Split;')
    index_owner = library / 'index_name.ada'
    view_owner = library / 'view_owner.ada'
    write(index_owner, 'package Decl_View is N : constant Integer := 3; end Decl_View;')
    write(view_owner, 'package Index_Name is N : constant Integer := 4; end Index_Name;')
    source = app / 'client.adb'
    write(source, '''with Ada.Text_IO; with Model; with Model.Child; with Tools;
with Alias; with Helper; with Extras; with Other_Extras; with Consumer; with Split; with Decl_View; with Index_Name;
procedure Client is
    package Instance is new Tools (Model.Item);
    X : Model.Item := Instance.Identity (Model.Make);
    T : Model.Tagged_Item := (N => 4);
begin
    Helper;
    if Model.Value (X) /= 5 or else Model.Value (3) /= 4
        or else Model.Child.Extra /= 3 or else T.Next /= 5
        or else Alias.Seed /= 5 or else Extras.N + Other_Extras.N /= 8
        or else Consumer.N /= 7 or else Split.Value /= 11 or else Decl_View.N + Index_Name.N /= 7 then
        raise Program_Error;
    end if;
    begin raise Model.Failure; exception when Model.Failure => null; end;
    Ada.Text_IO.Put_Line ("source-free");
end Client;''')
    program = work / ('producer-program' + args.exe_suffix)
    run([args.ada, '-D', objects, '-o', program, bundle, split_api, split_body, index_owner, view_owner, source], environment)
    run([program], environment, expected='source-free\n')
    original = (objects / 'model.asi').read_bytes()
    # File IDs and process addresses must not affect serialization.
    run([args.adac, '-c', '-I', library, '-D', work / 'deterministic', library / 'model.adb'], environment)
    if original != (work / 'deterministic/model.asi').read_bytes():
        raise RuntimeError('semantic interface is not deterministic')
    package = work / 'relocated library artifacts'
    package.mkdir()
    for artifact in objects.rglob('*'):
        if artifact.is_file() and artifact.suffix in {'.asi', '.ali', '.ssa'}:
            destination = package / artifact.relative_to(objects)
            destination.parent.mkdir(parents=True, exist_ok=True)
            shutil.copy2(artifact, destination)
    # An object-group key is not a guessed Ada declaration name.
    write(package / 'split_impl.adb', 'invalid source named after an object group')
    bundle_text = bundle.read_text()
    shutil.rmtree(library)
    later = work / 'later source root'
    write(later / 'split.ads', 'invalid later source')
    write(later / 'split.adb', 'invalid later body')
    environment['ADA_INCLUDE_PATH'] = str(package) + os.pathsep + str(later)
    # --no-stdlib makes even the predefined library resolve exclusively through
    # packaged interfaces. No original library source directory remains.
    target = work / ('client-program' + args.exe_suffix)
    client_objects = work / 'client objects'
    command = [args.ada, '--no-stdlib', '-v', '-D', client_objects, '-o', target, source]
    built = run(command, environment)
    run([target], environment, expected='source-free\n')
    if 'model uses a compiled semantic interface' not in built.stderr:
        raise RuntimeError('driver did not reuse the compiled library')
    # This client has a full private representation, default expressions,
    # overloads, tagged primitives, generic body/subunit replay and bundle aliases.
    run([args.adac, '--no-stdlib', '-c', '-I', package, '-D', work / 'frontend', source], environment)
    second = run(command, environment)
    if 'client needs no compiling' not in second.stderr:
        raise RuntimeError('unchanged interface client was recompiled')
    # Explicit custom-named source groups retain precedence when an imported
    # compiled record also names that group as a dependency.
    explicit_bundle = library / 'custom_bundle.ada'
    explicit_bundle.parent.mkdir(parents=True, exist_ok=True)
    explicit_bundle.write_text(bundle_text)
    run([args.ada, '--no-stdlib', '-D', work / 'mixed objects', '-o', target, explicit_bundle, source], environment)
    run([target], environment, expected='source-free\n')
    shutil.rmtree(library)
    # Relocating the artifact package must not require source-path rewriting.
    relocated = work / 'moved artifacts again'
    package.rename(relocated)
    package = relocated
    environment['ADA_INCLUDE_PATH'] = str(package) + os.pathsep + str(later)
    run(command, environment)
    run([target], environment, expected='source-free\n')
    private_client = app / 'private_client.adb'
    write(private_client, '''with Model;
procedure Private_Client is X : Model.Item := Model.Make;
begin X.N := 1; end Private_Client;''')
    result = subprocess.run([str(args.adac), '--no-stdlib', '-c', '-I', str(package), '-D',
                             str(work / 'private'), str(private_client)], env=environment,
                            capture_output=True, text=True, timeout=30)
    if result.returncode != 1 or 'private_client.adb:3:' not in result.stderr:
        raise RuntimeError('imported private representation became publicly selectable: ' + result.stderr)

    def reject(diagnostic, include=None):
        extra = [] if include is None else ['-I', include]
        run([args.adac, '--no-stdlib', '-c', *extra, '-I', package, '-D', work / 'rejected', source],
            environment, diagnostic=diagnostic)

    interface = package / '.aida-interfaces/model.spec.asi'
    interface.write_bytes(b'unsupported\n')
    reject('unsupported semantic interface format')
    interface.write_bytes(original[:-1])
    reject('invalid or truncated semantic interface')
    payload = bytearray(original.split(b'\n', 2)[2])
    payload[8:24] = b'0' * 16
    interface.write_bytes(envelope(payload))
    reject('produced by a different compiler')
    payload = bytearray(original.split(b'\n', 2)[2])
    offset = 8 + struct.unpack_from('<Q', payload)[0]
    offset += 8 + struct.unpack_from('<Q', payload, offset)[0]  # Target ABI.
    offset += 8 + struct.unpack_from('<Q', payload, offset)[0]  # Unit metadata.
    offset += 8  # Source count.
    offset += 8 + struct.unpack_from('<Q', payload, offset)[0]
    struct.pack_into('<Q', payload, offset, (1 << 64) - 1)
    interface.write_bytes(envelope(payload))
    reject('invalid or truncated semantic interface')
    payload = bytearray(original.split(b'\n', 2)[2])
    offset = 8 + struct.unpack_from('<Q', payload)[0]
    length = struct.unpack_from('<Q', payload, offset)[0]
    payload[offset + 8:offset + 8 + length] = b'x' * length
    interface.write_bytes(envelope(payload))
    reject('produced for a different target ABI')
    interface.write_bytes((package / '.aida-interfaces/extras.spec.asi').read_bytes())
    reject("does not declare unit 'model'")
    interface.write_bytes(original)
    record = package / 'model.ali'
    original_record = record.read_bytes()
    record.unlink()
    reject('missing or stale compiled semantic interface')
    record.write_bytes(original_record)
    # Dependency fingerprints must agree with the complete selected closure.
    record.write_bytes(original_record.replace(b'dep ada ', b'dep absent_dependency '))
    reject('missing or stale compiled semantic interface')
    record.write_bytes(original_record)
    ir = package / 'model.ssa'
    original_ir = ir.read_bytes()
    ir.write_bytes(original_ir + b'\n# modified\n')
    reject('missing or stale semantic interface dependencies')
    ir.write_bytes(original_ir)
    # Earlier source roots beat interface roots. Changing that selected source
    # also invalidates a compiled child which was built against its old digest.
    write(app / 'model.ads', model.replace(':= 5;', ':= 99;'))
    write(app / 'model.adb', model_body)
    reject('missing or stale semantic interface dependencies', include=app)
    (app / 'model.ads').unlink()
    (app / 'model.adb').unlink()
    run(command, environment)
    run([target], environment, expected='source-free\n')
    # Rebuild and republish only a library implementation. Its specification
    # remains compatible with the compiled child; replay clients conservatively
    # depend on the body digest and must regenerate their frontend artifacts.
    write(library / 'model.ads', model)
    write(library / 'model.adb', model_body.replace('return X.N;', 'return X.N + 0;'))
    updated = work / 'updated model'
    run([args.adac, '--no-stdlib', '-c', '-I', package, '-D', updated, library / 'model.adb'], environment)
    for artifact in updated.rglob('*'):
        if artifact.is_file() and artifact.suffix in {'.asi', '.ali', '.ssa'}:
            destination = package / artifact.relative_to(updated)
            destination.parent.mkdir(parents=True, exist_ok=True)
            shutil.copy2(artifact, destination)
    shutil.rmtree(library)
    rebuilt = run(command, environment)
    if 'client needs no compiling' in rebuilt.stderr:
        raise RuntimeError('replaced library body did not invalidate replay client')
    run([target], environment, expected='source-free\n')
    print('source-free interfaces, relocation, semantics, cache and rejection controls passed')


if __name__ == '__main__':
    main()
