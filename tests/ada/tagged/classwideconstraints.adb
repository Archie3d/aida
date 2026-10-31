with Ada.Text_IO;
procedure ClasswideConstraints is
    procedure Run is
        function Identity (Value : Boolean) return Boolean is
        begin
            return Value;
        end Identity;
        package Model is
            type Root (Enabled : Boolean) is tagged record
                Value : Integer := 7;
            end record;
            type Child (First, Second : Boolean) is new Root (First) with record
                Extra : Integer := 13;
            end record;
        end Model;
        Captured : Boolean := Identity (True);
        subtype Enabled is Model.Root (Captured);
        subtype Enabled_Class is Enabled'Class;
        subtype Both is Model.Child (True, False);
        type Any_Link is access all Model.Root'Class;
        type Enabled_Link is access all Enabled'Class;
        type Both_Link is access all Both'Class;
        Good : aliased Model.Child (True, False);
        Bad : aliased Model.Child (False, True);
        Wrong_Second : aliased Model.Child (True, True);
        Source : Any_Link := Bad'Access;
        Target : Enabled_Link;
        Both_Target : Both_Link;
        Calls : Integer := 0;
        procedure Check (Condition : Boolean) is
        begin
            if not Condition then
                raise Program_Error;
            end if;
        end Check;
        procedure Accept_Access (Item : access Enabled'Class) is
        begin
            Calls := Calls + 1;
            if Item /= null then
                Check (Item.Enabled and then Item.Value = 7);
            end if;
        end Accept_Access;
        procedure Accept_Value (Item : Enabled'Class) is
        begin
            Calls := Calls + 1;
            Check (Item.Enabled);
        end Accept_Value;
        function Result return Enabled'Class is
        begin
            return Bad;
        end Result;
    begin
        Captured := False;
        Both_Target := Good'Access;
        begin
            Both_Target := Wrong_Second'Access;
            raise Program_Error;
        exception
            when Constraint_Error => null;
        end;
        Check (Both_Target.First and then not Both_Target.Second);
        Target := Good'Access;
        Check (Target.Enabled and then Target.Value = 7);
        begin
            Target := Bad'Access;
            raise Program_Error;
        exception
            when Constraint_Error => null;
        end;
        begin
            Target := Bad'Unchecked_Access;
            raise Program_Error;
        exception
            when Constraint_Error => null;
        end;
        begin
            Target := Enabled_Link (Source);
            raise Program_Error;
        exception
            when Constraint_Error => null;
        end;
        begin
            Accept_Access (Source);
            raise Program_Error;
        exception
            when Constraint_Error => null;
        end;
        begin
            Accept_Value (Bad);
            raise Program_Error;
        exception
            when Constraint_Error => null;
        end;
        Check (Calls = 0);
        Source := Good'Access;
        Accept_Access (Source);
        Accept_Value (Good);
        Source := null;
        Target := Enabled_Link (Source);
        Check (Target = null);
        begin
            declare
                Rejected : not null Enabled_Link := Enabled_Link (Source);
            begin
                raise Program_Error;
            end;
        exception
            when Constraint_Error => null;
        end;
        Accept_Access (Source);
        Check (Calls = 3);
        declare
            Wide_Good : Model.Root'Class := Good;
            Wide_Bad : Model.Root'Class := Bad;
            Kept : Enabled_Class := Wide_Good;
        begin
            Check (Wide_Good in Enabled'Class);
            Check (Wide_Bad not in Enabled'Class);
            Check (Kept.Enabled);
            begin
                Kept := Wide_Bad;
                raise Program_Error;
            exception
                when Constraint_Error => null;
            end;
            Check (Kept.Enabled);
            begin
                declare
                    Rejected : Enabled'Class := Wide_Bad;
                begin
                    raise Program_Error;
                end;
            exception
                when Constraint_Error => null;
            end;
            begin
                declare
                    Rejected : Enabled'Class := Result;
                begin
                    raise Program_Error;
                end;
            exception
                when Constraint_Error => null;
            end;
        end;
    end Run;
    task type Worker;
    task body Worker is
    begin
        Run;
    exception
        when others => Ada.Text_IO.Put_Line ("class-wide constraints worker failed");
    end Worker;
begin
    Run;
    declare
        Workers : array (1 .. 4) of Worker;
    begin
        null;
    end;
    Ada.Text_IO.Put_Line ("class-wide constraints passed");
end ClasswideConstraints;
