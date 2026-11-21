with Interfaces.C;

procedure InterfacesLongDoubleUnavailable is
    package C renames Interfaces.C;
    use C;
    Value : long_double;
    Width : Integer := C.long_double'Size;
begin
    Value := C.long_double (1.25);
end InterfacesLongDoubleUnavailable;
