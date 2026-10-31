with Ada.Finalization;
package Controlled_Result_Model is
    type Resource is record
        References : Integer;
        Value : Integer;
    end record;
    type Link is access Resource;
    Resources : Integer := 0;
    Objects : Integer := 0;
    Fail_Adjust : Integer := 0;
    Fail_Finalize : Boolean := False;
    type Guard is new Ada.Finalization.Controlled with record
        Data : Link := null;
    end record;
    overriding procedure Initialize (Object : in out Guard);
    overriding procedure Adjust (Object : in out Guard);
    overriding procedure Finalize (Object : in out Guard);
    function Make (Value : Integer) return Guard;
    type Pair is record
        Left, Right : Guard;
    end record;
    type Guards is array (Integer range <>) of Guard;
    type Matrix is array (Integer range <>, Integer range <>) of Guard;
    procedure Check (Condition : Boolean);
end Controlled_Result_Model;
