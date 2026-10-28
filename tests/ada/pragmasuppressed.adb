pragma Suppress (All_Checks);
procedure PragmaSuppressed is
    subtype Small is Integer range 1 .. 3;
    Value : Integer := 2;
    Object : aliased Integer := 3;
    type Pointer is access all Integer;
    P : Pointer := Object'Access;
    Result : Small := Value;
begin
    Result := P.all;
end PragmaSuppressed;
