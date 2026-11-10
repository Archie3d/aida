package body Ada.Synchronous_Task_Control is
    procedure Destroy (Slot : System.Address);
    pragma Import (C, Destroy, "__ada_suspension_destroy");
    procedure Set_State (Slot : System.Address; State : Boolean);
    pragma Import (C, Set_State, "__ada_suspension_set");
    function Read_State (Slot : System.Address) return Boolean;
    pragma Import (C, Read_State, "__ada_suspension_state");
    procedure Wait (Slot : System.Address);
    pragma Import (C, Wait, "__ada_suspension_wait");

    overriding procedure Finalize (S : in out Suspension_Object) is
    begin
        Destroy (S.Handle'Address);
    end Finalize;
    procedure Set_True (S : in out Suspension_Object) is
    begin
        Set_State (S.Handle'Address, True);
    end Set_True;
    procedure Set_False (S : in out Suspension_Object) is
    begin
        Set_State (S.Handle'Address, False);
    end Set_False;
    function Current_State (S : Suspension_Object) return Boolean is
    begin
        return Read_State (S.Handle'Address);
    end Current_State;
    procedure Suspend_Until_True (S : in out Suspension_Object) is
    begin
        Wait (S.Handle'Address);
    end Suspend_Until_True;
end Ada.Synchronous_Task_Control;
