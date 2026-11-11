with Ada.Text_IO;
procedure Accessarrayconstraints is
    type Pointer is access String;
    subtype Pair is Pointer (1 .. 2);
    Low, High : Integer := 3;
    subtype Captured is Pointer (Low .. High);
    type Holder (Length : Natural) is record
        Data : Pointer (1 .. Length);
    end record;
    procedure Check (Value : Boolean) is
    begin
        if not Value then
            raise Program_Error with "access array constraints";
        end if;
    end Check;
    procedure Accept_Pair (Value : Pair) is
    begin
        Check (Value = null or else Value.all = "ab");
    end Accept_Pair;
    function As_Pair (Value : Pointer) return Pair is
    begin
        return Value;
    end As_Pair;
    Good : Pointer := new String'("ab");
    Shifted : Pointer := new String'(3 .. 4 => 'x');
    Short : Pointer := new String'("x");
    Single : Pointer := new String'(3 .. 3 => 'z');
    P : Pair := null;
    C : Captured := Single;
    H : Holder (2) := (2, Good);
    Failures : Natural := 0;
    package Deferred is
        Text : constant String;
        type Record_Value (Length : Natural) is record
            Data : String (1 .. Length);
        end record;
        Item : constant Record_Value;
    private
        Text : constant String := "deferred";
        Item : constant Record_Value := (2, "ok");
    end Deferred;
begin
    Low := 9;
    High := 12;
    P := Good;
    Accept_Pair (P);
    Check (As_Pair (Good).all = "ab" and C.all = "z");
    Check (H.Data.all = "ab");
    Check (Deferred.Text = "deferred" and Deferred.Item.Data = "ok");
    begin
        P := Shifted;
    exception when Constraint_Error => Failures := Failures + 1;
    end;
    begin
        Accept_Pair (Short);
    exception when Constraint_Error => Failures := Failures + 1;
    end;
    begin
        P := As_Pair (Shifted);
    exception when Constraint_Error => Failures := Failures + 1;
    end;
    begin
        H.Data := Short;
    exception when Constraint_Error => Failures := Failures + 1;
    end;
    begin
        P := new String'(3 .. 4 => 'x');
    exception when Constraint_Error => Failures := Failures + 1;
    end;
    Check (Failures = 5);
    P := null;
    H.Data := null;
    Check (P = null and H.Data = null);
    Ada.Text_IO.Put_Line ("access array constraints passed");
end Accessarrayconstraints;
