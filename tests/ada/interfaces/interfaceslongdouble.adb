with Ada.Text_IO;
with Interfaces.C;

procedure InterfacesLongDouble is
    package C renames Interfaces.C;
    use type C.int;
    use type C.long_double;
    function Mixed (Tag : C.int; A, B, C1, D, E, F, G, H, I, J : C.long_double;
                    Tail : C.long) return C.long_double;
    pragma Import (C, Mixed, "aida_interfaces_long_double");
    type Callback is access function
        (Tag : C.int; A, B, C1, D, E, F, G, H, I, J : C.long_double; Tail : C.long) return C.long_double;
    pragma Convention (C, Callback);
    Indirect : Callback := Mixed'Access;

    function Discriminate (Value : C.long_double) return Boolean is
    begin
        return True;
    end Discriminate;
    function Discriminate (Value : C.double) return Boolean is
    begin
        return False;
    end Discriminate;
begin
    if C.long_double'Size /= 64 or else C.long_double'Machine_Mantissa /= 53
        or else not Discriminate (C.long_double'(1.25)) or else Discriminate (C.double'(1.25))
        or else Mixed (-17, 1.25, -2.5, 3.75, -4.0, 5.5, -6.25, 7.75, -8.5, 9.5, -10.5, 123456789) /= -1.0
        or else Indirect (-17, 1.25, -2.5, 3.75, -4.0, 5.5, -6.25, 7.75, -8.5, 9.5, -10.5, 123456789) /= -1.0
    then
        raise Program_Error;
    end if;
    Ada.Text_IO.Put_Line ("native Interfaces.C.long_double passed");
end InterfacesLongDouble;
