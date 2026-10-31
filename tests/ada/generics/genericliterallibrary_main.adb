with Ada.Text_IO;
with GenericLiteralLibrary;
procedure GenericLiteralLibrary_Main is
    use GenericLiteralLibrary;
    type Shade is new Color range Green .. Blue;
    package First is new Views (Shade);
    package Nested is new First.Inner;
    function Replacement return Shade is
    begin
        return Blue;
    end Replacement;
    package Explicit_View is new Views (Shade, Replacement);
    type Callback is access function return Shade;
    Pointer : Callback := Explicit_View.Value'Access;
    procedure Check is
        Red : Integer := 42;
        package Local is new Views (Color);
    begin
        if First.Value /= Shade'Base'Val (0) or Nested.Value /= Shade'Base'Val (0)
            or Nested.Second /= Shade'(Green)
            or Pointer.all /= Blue or Explicit_View.Value /= Blue or Local.Value /= Color'(GenericLiteralLibrary.Red)
            or First.Character_Value /= Glyph'('a') then
            raise Program_Error;
        end if;
        begin
            declare
                Constrained : Shade := First.Value;
            begin
                raise Program_Error;
            end;
        exception
            when Constraint_Error => null;
        end;
    end Check;
    task type Worker;
    task body Worker is
    begin
        Check;
    end Worker;
begin
    Check;
    declare
        Workers : array (1 .. 4) of Worker;
    begin
        null;
    end;
    Ada.Text_IO.Put_Line ("library enumeration literal defaults passed");
end GenericLiteralLibrary_Main;
