with Ada.Text_IO;
with GenericAbstractModel;
with GenericAbstractLibrary;
procedure GenericAbstractLibrary_Main is
    package Model renames GenericAbstractModel;
    use Model;
    function Read (Object : Model.Child; Extra : Integer := 5) return Integer is
    begin
        return 999;
    end Read;
    package Instance is new GenericAbstractLibrary (Model.Root);
    function Nested is new Instance.Nested_Score;
    package Concrete is new GenericAbstractLibrary (Model.Child, Read => Model.Read);
    function Concrete_Nested is new Concrete.Nested_Score;
    procedure Check is
        Object : Model.Child := (Value => 2);
    begin
        if Instance.Score (Object) /= 7 or Nested (Object) /= 10
            or Concrete.Score (Object) /= 7 or Concrete_Nested (Object) /= 10
            or Instance.Box_Score (Object) /= 8 or Concrete.Box_Score (Object) /= 8 then
            raise Program_Error;
        end if;
        Instance.Update (Object);
        if Object.Value /= 6 then
            raise Program_Error;
        end if;
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
    Ada.Text_IO.Put_Line ("library abstract formal subprograms passed");
end GenericAbstractLibrary_Main;
