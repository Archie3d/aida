with Ada.Finalization;
with Ada.Unchecked_Deallocation;
with Ada.Text_IO;
procedure ControlledAdjustment is
    procedure Run is
        type Resource is record
            References : Natural;
            Key : Positive;
        end record;
        type Link is access Resource;
        procedure Free is new Ada.Unchecked_Deallocation (Resource, Link);
        Resources, Objects, Roots, Next_Key : Integer := 0;
        Fail_Adjust_Key, Fail_Finalize_Key : Integer := 0;
        Fail_Root : Boolean := False;
        Attempts : array (1 .. 128) of Natural := (others => 0);
        Root_Attempts : Natural := 0;
        procedure Check (Condition : Boolean) is
        begin
            if not Condition then
                raise Constraint_Error with "adjustment accounting";
            end if;
        end Check;
        type Guard is new Ada.Finalization.Controlled with record
            Data : Link;
            Adjusted : Boolean := False;
        end record;
        overriding procedure Initialize (Object : in out Guard);
        overriding procedure Adjust (Object : in out Guard);
        overriding procedure Finalize (Object : in out Guard);
        procedure Initialize (Object : in out Guard) is
        begin
            Next_Key := Next_Key + 1;
            Object.Data := new Resource'(1, Next_Key);
            Resources := Resources + 1;
            Objects := Objects + 1;
        end Initialize;
        procedure Adjust (Object : in out Guard) is
        begin
            Check (Object.Data /= null and then Object.Data.References > 0);
            Attempts (Object.Data.Key) := Attempts (Object.Data.Key) + 1;
            if Object.Data.Key = Fail_Adjust_Key then
                raise Storage_Error with "user Adjust";
            end if;
            Object.Data.References := Object.Data.References + 1;
            Object.Adjusted := True;
            Objects := Objects + 1;
        end Adjust;
        procedure Finalize (Object : in out Guard) is
            Key : Positive := Object.Data.Key;
        begin
            Check (Object.Data.References > 0);
            Object.Data.References := Object.Data.References - 1;
            if Object.Data.References = 0 then
                Free (Object.Data);
                Resources := Resources - 1;
            end if;
            Object.Data := null;
            Objects := Objects - 1;
            if Key = Fail_Finalize_Key then
                raise Storage_Error with "user Finalize";
            end if;
        end Finalize;
        type Guards is array (Positive range <>) of Guard;
        type Bundle is new Ada.Finalization.Controlled with record
            Parts : Guards (1 .. 3);
            Adjusted : Boolean := False;
        end record;
        overriding procedure Initialize (Object : in out Bundle);
        overriding procedure Adjust (Object : in out Bundle);
        overriding procedure Finalize (Object : in out Bundle);
        procedure Initialize (Object : in out Bundle) is
        begin
            Roots := Roots + 1;
        end Initialize;
        procedure Adjust (Object : in out Bundle) is
        begin
            Root_Attempts := Root_Attempts + 1;
            if Fail_Root then
                raise Storage_Error;
            end if;
            Object.Adjusted := True;
            Roots := Roots + 1;
        end Adjust;
        procedure Finalize (Object : in out Bundle) is
        begin
            Roots := Roots - 1;
        end Finalize;
        procedure Check_Parts (Source, Target : Guards; Failed : Positive) is
        begin
            for I in Source'Range loop
                Check (Attempts (Source (I).Data.Key) = 1);
                Check (Target (I).Data = Source (I).Data);
                Check (Target (I).Adjusted = (I /= Failed));
                if I = Failed then
                    Check (Source (I).Data.References = 1);
                else
                    Check (Source (I).Data.References = 2);
                end if;
            end loop;
        end Check_Parts;
    begin
        -- Failure in any component must leave the other target components
        -- adjusted, including the containing controlled object's own Adjust.
        for Failed in 1 .. 3 loop
            declare
                Source, Target : Bundle;
                Caught : Boolean := False;
            begin
                Root_Attempts := 0;
                Fail_Adjust_Key := Source.Parts (Failed).Data.Key;
                begin
                    Target := Source;
                exception
                    when Program_Error => Caught := True;
                end;
                Check (Caught and Target.Adjusted and Root_Attempts = 1);
                Check_Parts (Source.Parts, Target.Parts, Failed);
                Check (Objects = 5 and Resources = 3 and Roots = 2);
                Fail_Adjust_Key := 0;
                Target := Source;
                Check (Objects = 6 and Roots = 2);
            end;
            Check (Objects = 0 and Resources = 0 and Roots = 0);
            declare
                Source, Target : Guards (1 .. 3);
                Caught : Boolean := False;
            begin
                Fail_Adjust_Key := Source (Failed).Data.Key;
                begin
                    Target := Source;
                exception
                    when Program_Error => Caught := True;
                end;
                Check (Caught);
                Check_Parts (Source, Target, Failed);
                Fail_Adjust_Key := 0;
            end;
            Check (Objects = 0 and Resources = 0);
            declare
                Source, Seed : Bundle;
                Target : Bundle'Class := Seed;
                Caught : Boolean := False;
            begin
                Root_Attempts := 0;
                Fail_Adjust_Key := Source.Parts (Failed).Data.Key;
                begin
                    Target := Bundle'Class (Source);
                exception
                    when Program_Error => Caught := True;
                end;
                Check (Caught and Bundle (Target).Adjusted and Root_Attempts = 1);
                Check_Parts (Source.Parts, Bundle (Target).Parts, Failed);
                Fail_Adjust_Key := 0;
            end;
            Check (Objects = 0 and Resources = 0 and Roots = 0);
        end loop;
        -- Initialization failures still finalize only completed parts and
        -- propagate out of the declaration, without changing the source.
        declare
            Source : Bundle;
            Caught : Boolean := False;
        begin
            Fail_Adjust_Key := Source.Parts (1).Data.Key;
            begin
                declare
                    Copy : Bundle := Source;
                begin
                    raise Constraint_Error;
                end;
            exception
                when Program_Error => Caught := True;
            end;
            Check (Caught and Objects = 3 and Resources = 3 and Roots = 1);
            Fail_Adjust_Key := 0;
        end;
        Check (Objects = 0 and Resources = 0 and Roots = 0);
        -- A failing containing Adjust leaves successful children owned.
        declare
            Source, Target : Bundle;
            Caught : Boolean := False;
        begin
            Fail_Root := True;
            begin
                Target := Source;
            exception
                when Program_Error => Caught := True;
            end;
            Check (Caught and Objects = 6 and Roots = 1);
            Fail_Root := False;
        end;
        Check (Objects = 0 and Resources = 0 and Roots = 0);
        -- A failed anonymous snapshot of overlapping slices must clean up
        -- its successful parts without finalizing or overwriting the target.
        declare
            Values : Guards (1 .. 4);
            First_Key : Positive := Values (1).Data.Key;
            Second_Key : Positive := Values (2).Data.Key;
            Caught : Boolean := False;
        begin
            Fail_Adjust_Key := Second_Key;
            begin
                Values (2 .. 4) := Values (1 .. 3);
            exception
                when Program_Error => Caught := True;
            end;
            Check (Caught and Objects = 4 and Resources = 4);
            for I in Values'Range loop
                Check (not Values (I).Adjusted and Values (I).Data.References = 1);
            end loop;
            Fail_Adjust_Key := 0;
            Values (2 .. 4) := Values (1 .. 3);
            Check (Values (2).Data.Key = First_Key and Values (3).Data.Key = Second_Key);
            Check (Objects = 4 and Resources = 3);
            Values := Values;
            Check (Objects = 4 and Resources = 3);
        end;
        Check (Objects = 0 and Resources = 0);
        -- Finalize failures complete other finalizations and prevent copying.
        declare
            Source, Target : Bundle;
            Caught : Boolean := False;
        begin
            Fail_Finalize_Key := Target.Parts (2).Data.Key;
            Root_Attempts := 0;
            begin
                Target := Source;
            exception
                when Program_Error => Caught := True;
            end;
            Check (Caught and Objects = 3 and Resources = 3 and Roots = 1);
            Check (Root_Attempts = 0);
            Fail_Finalize_Key := 0;
        end;
        Check (Objects = 0 and Resources = 0 and Roots = 0);
        declare
            type Pointer is access Bundle;
            procedure Release is new Ada.Unchecked_Deallocation (Bundle, Pointer);
            Heap : Pointer := new Bundle;
            Caught : Boolean := False;
        begin
            Fail_Finalize_Key := Heap.Parts (2).Data.Key;
            begin
                Release (Heap);
            exception
                when Program_Error => Caught := True;
            end;
            Check (Caught and Objects = 0 and Resources = 0 and Roots = 0);
            Fail_Finalize_Key := 0;
        end;
    end Run;
    task type Worker;
    task body Worker is
    begin
        Run;
    exception
        when others => Ada.Text_IO.Put_Line ("worker adjustment failed");
    end Worker;
begin
    Run;
    declare
        Workers : array (1 .. 4) of Worker;
    begin
        null;
    end;
    Ada.Text_IO.Put_Line ("controlled adjustment passed");
end ControlledAdjustment;
