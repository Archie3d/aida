with Ada.Finalization;
package Master_Model is
    function Remember (Id : Integer) return Integer;
    pragma Import (C, Remember, "rememberMaster");
    function Remember_Allocation (Id : Integer) return Integer;
    pragma Import (C, Remember_Allocation, "rememberCollection");
    procedure Finalized (Id : Integer);
    pragma Import (C, Finalized, "masterFinalized");
    procedure Check_Done (Id : Integer);
    pragma Import (C, Check_Done, "masterCheckDone");
    function Fail return Integer;
    pragma Import (C, Fail, "masterFail");
    type Guard is new Ada.Finalization.Limited_Controlled with record
        Id : Integer := 0;
    end record;
    overriding procedure Finalize (Object : in out Guard);
    Global : Guard := (Ada.Finalization.Limited_Controlled with Id => Remember (1));
    type Pointer is access Guard;
    Heap : Pointer := new Guard;
end Master_Model;
