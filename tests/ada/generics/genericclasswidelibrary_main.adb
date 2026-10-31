with Ada.Text_IO;
with GenericClasswideModel;
with GenericClasswideLibrary;
procedure GenericClasswideLibrary_Main is
    package Model renames GenericClasswideModel;
    subtype Wide is Model.Root'Class;
    package Instance is new GenericClasswideLibrary (Wide);
    function Nested_Score is new Instance.Nested_Score (Model.Root'Class);
    procedure Check is
        Object : Wide := Model.Child'(Model.Root'(Value => 2) with Extra => 10);
        Duplicate : Wide := Instance.Clone (Object);
        Caught : Boolean := False;
    begin
        if Instance.Score (Duplicate) /= 12 or Nested_Score (Duplicate) /= 12 then
            raise Program_Error;
        end if;
        Instance.Reset (Object);
        if Instance.Score (Object) /= 28 then
            raise Program_Error;
        end if;
        begin
            Instance.Renamed_Reset (Object);
        exception
            when Program_Error => Caught := True;
        end;
        if not Caught or Instance.Score (Object) /= 28 then
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
    Ada.Text_IO.Put_Line ("library class-wide generic actuals passed");
end GenericClasswideLibrary_Main;
