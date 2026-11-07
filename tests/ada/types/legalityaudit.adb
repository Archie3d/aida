with Ada.Text_IO;
procedure LegalityAudit is
    procedure Check is
        type Colour is (Red, Green);
        type Derived_Colour is new Colour;
        type Number is range 0 .. 20;
        type Real_Number is digits 6;
        type Fixed_Number is delta 0.125 range -10.0 .. 10.0;
        type Values is array (Positive range <>) of Integer;
        type Link;
        type Link_Pointer is access all Link;
        type Link (Target : access Integer) is limited record
            Value : Integer := Target.all;
        end record;
        Value : aliased Integer := 7;
        Object : aliased Link (Value'Access);
        Alias : Link renames Object;
        type Pointer is access all Integer;
        Converted : Pointer := Pointer (Alias.Target);
        type Node is limited record
            Self : access Node := Node'Access;
        end record;
        Item : Node;
        type Rec (Length : Positive := 2) is record
            Text : String (1 .. Length) := (others => 'X');
            Bits : Natural := Rec'Size;
        end record;
        R : Rec;
        Larger : Rec (5);
        generic
            with procedure Action (X : in out Integer) is null;
        package Generic_View is
            procedure Run (X : in out Integer) renames Action;
        end Generic_View;
        package Default_View is new Generic_View;
        procedure Increment (X : in out Integer) is
        begin
            X := X + 1;
        end Increment;
        package Explicit_View is new Generic_View (Increment);
        type Callback is access procedure (X : in out Integer);
        Call : Callback := Explicit_View.Run'Access;
        procedure From_Parameter (P : access Link) is
            Borrowed : Pointer := Pointer (P.Target);
        begin
            if Borrowed.all /= 7 then
                raise Program_Error;
            end if;
        end From_Parameter;
        procedure Nested is
            type Inner_Pointer is access all Integer;
            Inner : Inner_Pointer := Inner_Pointer (Object.Target);
            Local_Pointer : Link_Pointer := Object'Access;
            Same_Level : Pointer := Pointer (Local_Pointer.Target);
        begin
            if Inner.all /= 7 or else Same_Level.all /= 7 then
                raise Program_Error;
            end if;
        end Nested;
    begin
        Default_View.Run (Value);
        if Converted.all /= 7 or else R.Text /= "XX" or else R.Bits /= R'Size or else Larger.Bits /= Larger'Size
            or else Fixed_Number'Aft /= 1
            or else Fixed_Number'Fore < 2 or else Fixed_Number'Small <= 0.0
            or else Fixed_Number'Delta <= 0.0 then
            raise Program_Error;
        end if;
        From_Parameter (Object'Access);
        Item.Self.all.Self := null;
        Nested;
        Call.all (Value);
        if Value /= 8 then
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
        A, B, C, D : Worker;
    begin
        null;
    end;
    Ada.Text_IO.Put_Line ("legality audit passed");
end LegalityAudit;
