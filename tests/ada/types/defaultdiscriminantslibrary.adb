with Ada.Text_IO; use Ada.Text_IO;
with DefaultDiscriminantLibrary; use DefaultDiscriminantLibrary;
procedure DefaultDiscriminantsLibrary is
    X : Item;
    Fixed : Item (False);
begin
    Set (X, (True, 2));
    Fill (Global);
    if not X.D or X.Value /= 2 or not Global.D or Global.Value /= 42
        or X'Constrained or Global'Constrained or not Is_Constrained (X) then
        raise Program_Error;
    end if;
    begin
        Set (Fixed, Global);
        raise Program_Error;
    exception
        when Constraint_Error =>
            if Fixed.D or Fixed.Value /= 1 then
                raise Program_Error;
            end if;
    end;
    Put_Line ("library discriminants ok");
end DefaultDiscriminantsLibrary;
