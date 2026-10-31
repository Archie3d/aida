with Ada.Unchecked_Deallocation;
with Controlled_Result_Model; use Controlled_Result_Model;
with Ada.Text_IO; use Ada.Text_IO;
procedure ControlledAllocationFailures is
    procedure Fail_Initialization is
        Attempts : Integer := 0;
        type Falling is new Guard with null record;
        overriding procedure Initialize (Object : in out Falling) is
        begin
            Attempts := Attempts + 1;
            if Attempts = 2 then
                raise Constraint_Error;
            end if;
            Initialize (Guard (Object));
        end Initialize;
        type Holder is record
            First, Second : Falling;
        end record;
        type Link is access Holder;
    begin
        declare
            Object : Link := new Holder;
        begin
            raise Program_Error;
        end;
    exception
        when Constraint_Error => Check (Objects = 0 and Resources = 0);
    end Fail_Initialization;
    procedure Fail_Copy is
        type Pair_Link is access Pair;
        Local : Pair;
    begin
        Fail_Adjust := 2;
        declare
            Object : Pair_Link := new Pair'(Local);
        begin
            raise Constraint_Error;
        end;
    exception
        when Program_Error => Check (Objects = 2 and Resources = 2);
    end Fail_Copy;
    procedure Fail_Free is
        type Guard_Link is access Guard;
        procedure Free is new Ada.Unchecked_Deallocation (Guard, Guard_Link);
        Object : Guard_Link := new Guard;
    begin
        Fail_Finalize := True;
        begin
            Free (Object);
            raise Constraint_Error;
        exception
            when Program_Error =>
                Check (Objects = 0 and Resources = 0);
                Object := null;
        end;
    end Fail_Free;
    procedure Fail_Collection is
        type Guard_Link is access Guard;
        A, B : Guard_Link := new Guard;
    begin
        Fail_Finalize := True;
    end Fail_Collection;
    procedure Unwind is
        type Guard_Link is access Guard;
        Object : Guard_Link := new Guard;
    begin
        raise Constraint_Error;
    end Unwind;
begin
    Fail_Initialization;
    Fail_Copy;
    Check (Objects = 0 and Resources = 0);
    Fail_Free;
    begin
        Fail_Collection;
        raise Constraint_Error;
    exception
        when Program_Error => Check (Objects = 0 and Resources = 0);
    end;
    begin
        Unwind;
        raise Program_Error;
    exception
        when Constraint_Error => Check (Objects = 0 and Resources = 0);
    end;
    Put_Line ("controlled allocation failures passed");
end ControlledAllocationFailures;
