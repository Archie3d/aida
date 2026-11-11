# B394A01 local foundation adapter

`f394a00.a` is a local derivative of the pinned upstream foundation, with its
rights/disclaimer retained. It is not an upstream correction. Only B394A01's
`foundation_overrides` policy selects it; the report and console label that grade.
The original test, foundation, manifest hashes and raw observation stay intact.

The base is `support/f394a00.a` from
[simonjwright/ACATS revision 2785c30](https://github.com/simonjwright/ACATS/blob/2785c30c1f8577d97ac7b3043882f6818781e9c5/support/f394a00.a):

- Git blob: `81bdfaefb2ddc57f97855ee2c562dd014aa4264f`
- SHA-256: `c854401d7b25cc9d4e42b1e77e77a49b3d1fcc9b1ac38130de0753b8267c8237`

On 2026-11-11, official ACAA [F394A00 revision 1.1](http://www.ada-auth.org/cgi-bin/cvsweb.cgi/acats/support/f394a00.a?rev=1.1&content-type=text/plain)
and [B394A01 revision 1.3](http://www.ada-auth.org/cgi-bin/cvsweb.cgi/acats/b3/b394a01.a?rev=1.3&content-type=text/plain)
were compared with the pinned files: their text matches after CRLF-to-LF
conversion. The [foundation history](http://www.ada-auth.org/cgi-bin/cvsweb.cgi/acats/support/f394a00.a)
has only revision 1.1; the [test history](http://www.ada-auth.org/cgi-bin/cvsweb.cgi/acats/b3/b394a01.a)
ends at 1.3. The mirror's original 4.1 test at `39d6385865a54a152359157669ed96946eb4a245`
also names both missing types. No corrected upstream pair was found.

The only additions are a comment and these two private extensions and full views:

```ada
    type Non_Lim_Intf_Priv is new Non_Lim_Interface with private;
    type Lim_Intf_Priv is limited new Lim_Interface with private;
    -- In the private part:
    type Non_Lim_Intf_Priv is new Non_Lim_Interface with null record;
    type Lim_Intf_Priv is limited new Lim_Interface with null record;
```

They implement the private-extension cases already named by the test. They are
legal ancestors and are not interfaces, so using them as progenitors must fail.
All 87 required and ten optional test sites then receive the intended semantic
error. Unknown-name diagnostics remain forbidden. The independent
[legal control](../../../ada/tasks/interfaceancestry.adb) exercises equivalent
private-extension ancestors and generic actuals in whole/per-unit execution.
