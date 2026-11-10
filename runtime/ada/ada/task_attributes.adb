with Ada.Finalization;
with Ada.Unchecked_Deallocation;
with System;
package body Ada.Task_Attributes is
    use type System.Address;
    function Create return System.Address;
    pragma Import (C, Create, "__ada_task_attributes_create");
    procedure Close (Key : System.Address);
    pragma Import (C, Close, "__ada_task_attributes_close");
    function Acquire (Key : System.Address; T : Task_Id) return System.Address;
    pragma Import (C, Acquire, "__ada_task_attribute_acquire");
    procedure Release (Token : System.Address);
    pragma Import (C, Release, "__ada_task_attribute_release");
    function Read_Value (Token : System.Address) return Attribute_Handle;
    pragma Import (C, Read_Value, "__ada_task_attribute_get");
    procedure Write_Value (Token : System.Address; Val : Attribute_Handle);
    pragma Import (C, Write_Value, "__ada_task_attribute_put");
    procedure Free is new Ada.Unchecked_Deallocation (Attribute, Attribute_Handle);

    type Instance_Control is new Ada.Finalization.Limited_Controlled with record
        Key : System.Address := System.Null_Address;
    end record;
    overriding procedure Initialize (Object : in out Instance_Control);
    overriding procedure Finalize (Object : in out Instance_Control);
    overriding procedure Initialize (Object : in out Instance_Control) is
    begin
        Object.Key := Create;
    end Initialize;
    overriding procedure Finalize (Object : in out Instance_Control) is
    begin
        Close (Object.Key);
    end Finalize;
    State : Instance_Control;

    type Lock_Guard is new Ada.Finalization.Limited_Controlled with record
        Token : System.Address := System.Null_Address;
    end record;
    overriding procedure Finalize (Object : in out Lock_Guard);
    overriding procedure Finalize (Object : in out Lock_Guard) is
    begin
        Release (Object.Token);
    end Finalize;

    function Ensure (Token : System.Address) return Attribute_Handle is
        Result : Attribute_Handle := Read_Value (Token);
    begin
        if Result = null then
            Result := new Attribute'(Initial_Value);
            Write_Value (Token, Result);
        end if;
        return Result;
    end Ensure;
    function Value (T : Task_Id := Current_Task) return Attribute is
        Guard : Lock_Guard;
        Result : Attribute_Handle;
    begin
        Guard.Token := Acquire (State.Key, T);
        Result := Read_Value (Guard.Token);
        if Result = null then return Initial_Value; end if;
        return Result.all;
    end Value;
    function Reference (T : Task_Id := Current_Task) return Attribute_Handle is
        Guard : Lock_Guard;
    begin
        Guard.Token := Acquire (State.Key, T);
        return Ensure (Guard.Token);
    end Reference;
    procedure Set_Value (Val : in Attribute; T : in Task_Id := Current_Task) is
        Guard : Lock_Guard;
        Target : Attribute_Handle;
    begin
        Guard.Token := Acquire (State.Key, T);
        Target := Ensure (Guard.Token);
        Target.all := Val;
    end Set_Value;
    procedure Reinitialize (T : in Task_Id := Current_Task) is
        Guard : Lock_Guard;
        Target : Attribute_Handle;
    begin
        Guard.Token := Acquire (State.Key, T);
        Target := Read_Value (Guard.Token);
        Write_Value (Guard.Token, null);
        Free (Target);
    end Reinitialize;
end Ada.Task_Attributes;
