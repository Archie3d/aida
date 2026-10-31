with Ada.Text_IO;
with Enclosing_Model;
procedure EnclosingNames is
    procedure Check (Condition : Boolean) is
    begin
        if not Condition then raise Program_Error; end if;
    end Check;
    procedure Exercise (Seed : Integer) is
        Total : Integer := 0;
    begin
        Region : declare
            Value : Integer := Seed;
            subtype Small is Integer range 1 .. 20;
            type Vector is array (1 .. 2) of Integer;
            Data : Vector := (Seed, Seed + 1);
            type Item is record
                Field : Integer := Seed;
            end record;
            Object : Item;
            Failure : exception;
            Alias : Integer renames Region.Value;
            function Read return Integer is
                Value : Integer := 99;
            begin
                Check (Read.Value = 99);
                return Region.Value + EnclosingNames.Exercise.Seed;
            end Read;
            function Pick return Integer is
            begin
                return Region.Value;
            end Pick;
            function Pick return Boolean is
            begin
                return True;
            end Pick;
            procedure Inner (Value : Integer := Region.Value) is
                type Item is record
                    Other : Integer;
                end record;
                Local : Region.Item := (Field => Value);
                Count : Region.Small := Value;
            begin
                Check (Inner.Value = Seed and Local.Field = Seed and Count = Seed);
                Region.Data (2) := Region.Object.Field + 2;
                Region.Alias := Value + 1;
                Exercise.Total := Region.Pick;
                Check (Region.Pick);
                Check (EnclosingNames.Exercise.Region.Value = Seed + 1);
                begin
                    raise Region.Failure;
                exception
                    when Region.Failure => null;
                end;
            end Inner;
            type Callback is access function return Integer;
            Saved : Callback := Region.Read'Access;
        begin
            Region.Inner;
            Check (Value = Seed + 1 and Data (2) = Seed + 2 and Total = Seed + 1);
            Check (Saved.all = 2 * Seed + 1);
            Check (Region."=" (Data, Data));
            Counted : for Index in 1 .. 3 loop
                Check (Counted.Index = Index);
                Region.Value := Region.Value + Counted.Index;
                Nested_Loop : for Inner_Index in 1 .. 2 loop
                    Check (Nested_Loop.Inner_Index = Inner_Index);
                    Nested_Block : declare
                        Captured : Integer := Counted.Index + Nested_Loop.Inner_Index;
                    begin
                        Check (Nested_Block.Captured = Index + Inner_Index);
                        Check (Region.Nested_Block.Captured = Index + Inner_Index);
                    end Nested_Block;
                end loop Nested_Loop;
            end loop Counted;
            Check (Region.Value = Seed + 7);
        end Region;
        Generic_Region : declare
            Value : Integer := Seed;
            generic
                Number : Integer;
            package Template is
                Initial : Integer := Template.Number;
                procedure Verify;
            end Template;
            package body Template is
                Saved : Integer := Generic_Region.Value;
                Failure : exception;
                procedure Verify is
                    Value : Integer := 0;
                begin
                    Check (Template.Saved = Seed and Template.Initial = Seed);
                    Check (Template.Number = Seed and Generic_Region.Value = Seed);
                    begin
                        raise Template.Failure;
                    exception
                        when Template.Failure => Value := 1;
                    end;
                    Check (Value = 1);
                end Verify;
            end Template;
            package Instance is new Template (Seed);
            generic
                Number : Integer;
            procedure Action;
            procedure Action is
                Local : Integer := Action.Number;
            begin
                Action.Local := Action.Local + Generic_Region.Value;
                Check (Action.Local = 2 * Seed);
            end Action;
            procedure Apply is new Action (Seed);
        begin
            Instance.Verify;
            Apply;
        end Generic_Region;
    end Exercise;
    task type Worker;
    task body Worker is
    begin
        Exercise (3);
    exception
        when others => Ada.Text_IO.Put_Line ("enclosing-name worker failed");
    end Worker;
begin
    Exercise (2);
    Enclosing_Model.Verify;
    declare
        Workers : array (1 .. 4) of Worker;
    begin
        null;
    end;
    Ada.Text_IO.Put_Line ("enclosing names passed");
end EnclosingNames;
