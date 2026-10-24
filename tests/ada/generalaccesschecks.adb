with Ada.Text_IO; use Ada.Text_IO;
procedure GeneralAccessChecks is
    type Link is access all Integer;
    Global : aliased Integer := 7;
    generic
    package Template is
        function Local return Link;
        function Convert return Link;
    end Template;
    package body Template is
        Value : aliased Integer := 8;
        type Inner_Link is access all Integer;
        P : Inner_Link := Value'Access;
        function Local return Link is
        begin
            return Value'Access;
        end Local;
        function Convert return Link is
        begin
            return Link (P);
        end Convert;
    end Template;
    package Good is new Template;
    procedure Check (Condition : Boolean) is
    begin
        if not Condition then
            raise Program_Error;
        end if;
    end Check;
    type Variant (Present : Boolean := True) is record
        Common : aliased Integer := 42;
        case Present is
            when True => Value : aliased Integer := 17;
            when False => null;
        end case;
    end record;
    A : aliased Variant;
    B : aliased Variant (True);
    V : Link := A.Common'Access;
    type Variant_Link is access all Variant;
    P : Variant_Link := A'Access;
    Fixed : Variant_Link := B'Access;
    type True_Link is access all Variant (True);
    Constrained_View : True_Link := True_Link (P);
    type Variant_Constant is access constant Variant;
    Constant_View : Variant_Constant := Variant_Constant (P);
    Calls : Integer := 0;
    function Selected return Variant_Link is
    begin
        Calls := Calls + 1;
        return P;
    end Selected;
    procedure Replace (Object : in out Variant) is
    begin
        Check (not Object'Constrained);
        Object := (False, Common => 71);
    end Replace;
begin
    Check (Good.Local.all = 8 and Good.Convert.all = 8);
    Check (not A'Constrained and not P.all'Constrained and Fixed.all'Constrained);
    Selected.all := (False, Common => 43);
    Check (Calls = 1 and not A.Present and V.all = 43);
    Check (not Selected.all'Constrained and Calls = 2);
    Check (not P.all'Constrained and Constant_View.all'Constrained);
    begin
        Constrained_View := True_Link (P);
        raise Program_Error;
    exception
        when Constraint_Error => null;
    end;
    Constrained_View := True_Link (Variant_Link (null));
    Check (Constrained_View = null);
    Replace (P.all);
    Check (not A.Present and V.all = 71);
    P.all := (True, Common => 42, Value => 99);
    Check (A.Present and A.Value = 99);
    begin
        Fixed.all := (False, Common => 0);
        raise Program_Error;
    exception
        when Constraint_Error => Check (B.Present);
    end;
    declare
        package Deep is new Template;
        P : Link := Global'Access;
    begin
        begin
            P := Deep.Local;
            raise Constraint_Error;
        exception
            when Program_Error => Check (P.all = 7);
        end;
        begin
            P := Deep.Convert;
            raise Constraint_Error;
        exception
            when Program_Error => Check (P.all = 7);
        end;
    end;
    Put_Line ("accessibility and aliased constraints passed");
end GeneralAccessChecks;
