with Ada.Text_IO; use Ada.Text_IO;
with Controlled_Result_Model; use Controlled_Result_Model;
procedure ControlledAncestor is
    Finalized : Integer := 0;
    type Child is new Guard with record
        Extra : Guard;
        Marker : Integer := 123;
    end record;
    overriding function Make (Value : Integer) return Child is
        Object : Child;
    begin
        Object.Data.Value := Value;
        return Object;
    end Make;
    overriding procedure Finalize (Object : in out Child) is
    begin
        Finalized := Finalized + 1;
        Controlled_Result_Model.Finalize (Guard (Object));
    end Finalize;
    procedure Copy (Target : in out Guard; Source : Guard) is
    begin
        Target := Source;
    end Copy;
begin
    declare
        A, B : Child;
    begin
        B.Data.Value := 42;
        A.Extra.Data.Value := 7;
        Guard (A) := Guard (B);
        Check (Finalized = 0 and Objects = 4 and Resources = 3);
        Check (A.Data.Value = 42 and A.Extra.Data.Value = 7 and A.Marker = 123);
        Copy (Guard (A), Guard (B));
        Guard (A) := Guard (A);
        Check (Finalized = 0 and Objects = 4 and Resources = 3);
    end;
    Check (Finalized = 2 and Objects = 0 and Resources = 0);
    Put_Line ("controlled ancestor assignment passed");
end ControlledAncestor;
