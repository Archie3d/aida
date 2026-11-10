with Ada.Text_IO; use Ada.Text_IO;
with Ada.Finalization;
with Ada.Task_Identification; use Ada.Task_Identification;
with System; use type System.Address;
procedure RequeueEntries is
    Original_Caller : constant Task_Id := Current_Task;
    X : Integer := 0;
    Seen : System.Address := System.Null_Address;
    Aliased_Value : aliased Integer := 4;
    Finalized, Index_Calls : Natural := 0;
    pragma Atomic (Finalized);
    pragma Atomic (Index_Calls);
    function Next_Index return Integer is
    begin Index_Calls := Index_Calls + 1; return 2; end Next_Index;
    type Item is new Ada.Finalization.Limited_Controlled with null record;
    overriding procedure Finalize (Object : in out Item) is
    begin Finalized := Finalized + 1; end Finalize;
    task Last is entry Work (C : in out Integer); end Last;
    task body Last is
    begin
        accept Work (C : in out Integer) do
            if Work'Caller /= Original_Caller then raise Program_Error; end if;
            C := C + 5;
        end Work;
    end Last;
    protected Other is
        entry Work (N : in out Integer);
        entry Fail (N : in out Integer);
    end Other;
    protected body Other is
        entry Work (N : in out Integer) when True is
        begin N := N + 7; end Work;
        entry Fail (N : in out Integer) when True is
        begin N := 666; raise Constraint_Error; end Fail;
    end Other;
    protected Gate is
        entry Step (N : in out Integer);
        entry Finish (M : in out Integer);
        entry Cross (K : in out Integer);
        entry Fail (K : in out Integer);
        entry No_Args (N : in out Integer);
        entry Done;
        entry Check_Address (Value : Integer; Address : in out System.Address);
        entry Access_Start (Value : access Integer; Address : in out System.Address);
        entry Access_End (Value : access Integer; Address : in out System.Address);
        function Completed return Natural;
    private Completions : Natural := 0;
    end Gate;
    protected body Gate is
        entry Step (N : in out Integer) when True is
        begin N := N + 3; requeue Finish; raise Program_Error; end Step;
        entry Finish (M : in out Integer) when True is
        begin M := M + 4; requeue Last.Work; raise Program_Error; end Finish;
        entry Cross (K : in out Integer) when True is
        begin requeue Other.Work; raise Program_Error; end Cross;
        entry Fail (K : in out Integer) when True is
        begin K := 999; requeue Other.Fail; end Fail;
        entry No_Args (N : in out Integer) when True is
        begin N := N + 8; requeue Done; end No_Args;
        entry Done when True is
        begin Completions := Completions + 1; end Done;
        entry Check_Address (Value : Integer; Address : in out System.Address) when True is
        begin if Address /= Value'Address then raise Program_Error; end if; end Check_Address;
        entry Access_Start (Value : access Integer; Address : in out System.Address) when True is
        begin
            Address := Value'Address;
            Value.all := Value.all + 1;
            requeue Access_End;
        end Access_Start;
        entry Access_End (Value : access Integer; Address : in out System.Address) when True is
        begin
            if Address /= Value'Address then raise Program_Error; end if;
            Value.all := Value.all + 2;
        end Access_End;
        function Completed return Natural is begin return Completions; end Completed;
    end Gate;
    task Middle is entry Work (1 .. 2) (B : in out Integer); end Middle;
    task body Middle is
    begin
        accept Work (2) (B : in out Integer) do
            if Finalized /= 1 then raise Program_Error; end if;
            B := B + 2;
            requeue Gate.Step;
        end Work;
    end Middle;
    task First is
        entry Work (A : in out Integer);
        entry Identity (Value : Integer; Address : in out System.Address);
    end First;
    task body First is
    begin
        accept Work (A : in out Integer) do
            A := A + 1;
            declare
                Object : Item;
                A : Integer := 99;
            begin
                requeue Middle.Work (Next_Index);
            end;
        end Work;
        accept Identity (Value : Integer; Address : in out System.Address) do
            Address := Value'Address;
            requeue Gate.Check_Address;
        end Identity;
    end First;
begin
    First.Work (X);
    if X /= 15 or Finalized /= 1 or Index_Calls /= 1 then raise Program_Error; end if;
    Gate.Cross (X);
    if X /= 22 then raise Program_Error; end if;
    Gate.No_Args (X);
    if X /= 30 or Gate.Completed /= 1 then raise Program_Error; end if;
    First.Identity (42, Seen);
    Gate.Access_Start (Aliased_Value'Access, Seen);
    if Aliased_Value /= 7 then raise Program_Error; end if;
    Gate.Access_Start (new Integer'(4), Seen);
    begin Gate.Fail (X); raise Program_Error;
    exception when Constraint_Error => null;
    end;
    if X /= 30 then raise Program_Error; end if;
    -- The source task can finish while its redirected call is queued on one
    -- of its local protected objects. Finalization clears that queue safely.
    declare
        task Server is entry Work (N : in out Integer); end Server;
        task body Server is
            protected Local is entry Wait (N : in out Integer); end Local;
            protected body Local is
                entry Wait (N : in out Integer) when False is begin N := 99; end Wait;
            end Local;
        begin
            accept Work (N : in out Integer) do N := 40; requeue Local.Wait; end Work;
        end Server;
    begin
        begin Server.Work (X); raise Constraint_Error;
        exception when Program_Error => null;
        end;
        if X /= 30 then raise Constraint_Error; end if;
    end;
    declare
        Touched : Boolean := False;
        pragma Atomic (Touched);
        task type Listener is entry Touch; end Listener;
        task body Listener is
        begin accept Touch do Touched := True; end Touch; end Listener;
        Object : Listener;
        type Reference is access Listener;
        Pointer : Reference := new Listener;
        task Proxy is
            entry Send (Target : in out Listener);
            entry Indirect;
            entry Again;
        end Proxy;
        task body Proxy is
        begin
            accept Send (Target : in out Listener) do requeue Target.Touch; end Send;
            accept Indirect do requeue Again; end Indirect;
            accept Again do
                declare Local : Reference := Pointer;
                begin requeue Local.all.Touch; end;
            end Again;
        end Proxy;
    begin
        Proxy.Send (Object);
        if not Touched then raise Program_Error; end if;
        Touched := False;
        Proxy.Indirect;
        if not Touched then raise Program_Error; end if;
    end;
    Put_Line ("requeue entries OK");
end RequeueEntries;
