with Ada.Environment_Variables;
with Ada.Text_IO;
procedure EnvironmentVariables is
    package Env renames Ada.Environment_Variables;
    Count : Natural := 0;
    Found : Boolean := False;
    procedure Check (Value : Boolean) is
    begin
        if not Value then
            raise Program_Error;
        end if;
    end Check;
    procedure Visit (Name, Value : String) is
    begin
        Count := Count + 1;
        if Name = "AIDA_PHASE7_ENV" then
            Check (Value = "second");
            Found := True;
        end if;
    end Visit;
    Caught : Boolean := False;
begin
    Env.Clear ("AIDA_PHASE7_ENV");
    Check (not Env.Exists ("AIDA_PHASE7_ENV"));
    begin
        Ada.Text_IO.Put_Line (Env.Value ("AIDA_PHASE7_ENV"));
    exception
        when Constraint_Error => Caught := True;
    end;
    Check (Caught);
    Env.Set ("AIDA_PHASE7_ENV", "");
    Check (Env.Exists ("AIDA_PHASE7_ENV") and Env.Value ("AIDA_PHASE7_ENV") = "");
    Env.Set ("AIDA_PHASE7_ENV", "first");
    Env.Set ("AIDA_PHASE7_ENV", "second");
    Check (Env.Value ("AIDA_PHASE7_ENV") = "second");
    Env.Iterate (Visit'Access);
    Check (Found and Count > 0);
    Caught := False;
    begin
        Env.Set ("bad=name", "value");
    exception
        when Constraint_Error => Caught := True;
    end;
    Check (Caught and not Env.Exists ("bad=name"));
    Caught := False;
    begin
        Env.Set ("AIDA_PHASE7_ENV", "a" & Character'Val (0) & "b");
    exception
        when Constraint_Error => Caught := True;
    end;
    Check (Caught);
    Caught := False;
    begin
        Env.Iterate (null);
    exception
        when Constraint_Error => Caught := True;
    end;
    Check (Caught);
    Env.Clear;
    Count := 0;
    Env.Iterate (Visit'Access);
    Check (Count = 0 and not Env.Exists ("AIDA_PHASE7_ENV"));
    Env.Set ("AIDA_PHASE7_ENV", "restored");
    Check (Env.Value ("AIDA_PHASE7_ENV") = "restored");
    Ada.Text_IO.Put_Line ("environment variables passed");
end EnvironmentVariables;
