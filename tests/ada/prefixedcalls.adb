with Ada.Text_IO; use Ada.Text_IO;
with Prefixed_Model;
procedure PrefixedCalls is
    procedure Exercise is
        X : aliased Prefixed_Model.Root;
        Y : aliased Prefixed_Model.Child;
        Hidden : Prefixed_Model.Private_Root;
        type Wide is access all Prefixed_Model.Root'Class;
        P : Wide := Y'Access;
        Calls : Integer := 0;
        function Select_Object return Wide is
        begin
            Calls := Calls + 1;
            return P;
        end Select_Object;
        function Make return Prefixed_Model.Root is
        begin
            Calls := Calls + 1;
            return X;
        end Make;
        function Make return Prefixed_Model.Other is
            Object : Prefixed_Model.Other;
        begin
            return Object;
        end Make;
        function Identity (Object : Prefixed_Model.Root'Class) return Integer is
        begin
            return Object.Read;
        end Identity;
    begin
        -- No use clause: the type's region supplies the operations.
        X.Add;
        X.Add (Amount => 3);
        X.Add (True);
        if X.Read /= 10 or not X.Read then raise Program_Error; end if;
        X.Set (Value => 7);
        if X.Via_Access /= 7 or Hidden.Read /= 42 then raise Program_Error; end if;
        Y.Add (4);
        if Y.Read /= 15 or P.Read /= 15 or P.all.Read /= 15 then raise Program_Error; end if;
        if Select_Object.Via_Access /= 15 or Calls /= 1 then raise Program_Error; end if;
        Select_Object.Set (19);
        if Y.Value /= 19 or Calls /= 2 then raise Program_Error; end if;
        if P.Wide_Read (Amount => 3) /= 22 then raise Program_Error; end if;
        -- The result context selects both the operation and its overloaded prefix.
        declare
            Number : Integer := Make.Read;
            Letter : Character := Make.Read;
        begin
            if Number /= 7 or Letter /= 'Q' or Calls /= 3 then raise Program_Error; end if;
        end;
        if X.Copy.Read /= 7 or Identity (P.Copy) /= 29 then raise Program_Error; end if;
        if Prefixed_Model.Via_Access (P.all'Access) /= 29 then raise Program_Error; end if;
        P := null;
        begin
            X.Value := P.Read;
            raise Program_Error;
        exception
            when Constraint_Error => null;
        end;
        begin
            X.Value := P.Via_Access;
            raise Program_Error;
        exception
            when Constraint_Error => null;
        end;
        -- A defaulted call through a constant dereference is legal for mode in.
        declare
            type Constant_Wide is access constant Prefixed_Model.Root'Class;
            C : Constant_Wide := Y'Access;
        begin
            if C.Via_Access /= 29 then raise Program_Error; end if;
        end;
    end Exercise;
    function Increment (Value : Integer) return Integer is
    begin
        return Value + 1;
    end Increment;
    package Components is
        type Callback is access function (Value : Integer) return Integer;
        type Item is tagged record
            Apply : Callback := Increment'Access;
        end record;
        function Apply (Object : Item; Value : Boolean) return Integer;
    end Components;
    package body Components is
        function Apply (Object : Item; Value : Boolean) return Integer is
        begin
            return -1;
        end Apply;
    end Components;
    Component : Components.Item;
    protected Results is
        procedure Done;
        function Count return Integer;
    private
        Total : Integer := 0;
    end Results;
    protected body Results is
        procedure Done is
        begin
            Total := Total + 1;
        end Done;
        function Count return Integer is
        begin
            return Total;
        end Count;
    end Results;
begin
    Exercise;
    if Component.Apply (2) /= 3 then raise Program_Error; end if;
    declare
        task type Worker;
        task body Worker is
        begin
            Exercise;
            Results.Done;
        end Worker;
        Workers : array (1 .. 4) of Worker;
    begin
        null;
    end;
    if Results.Count /= 4 then raise Program_Error; end if;
    Put_Line ("prefixed calls passed");
end PrefixedCalls;
