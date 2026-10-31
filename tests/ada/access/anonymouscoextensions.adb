with Ada.Text_IO; use Ada.Text_IO;
with Ada.Finalization;
with Ada.Unchecked_Deallocation;
procedure AnonymousCoextensions is
    Live : Integer := 0;
    Fail : Boolean := False;
    package Resources is
        type Child is new Ada.Finalization.Controlled with record
            Value : Integer := 7;
        end record;
        overriding procedure Initialize (Object : in out Child);
        overriding procedure Finalize (Object : in out Child);
    end Resources;
    package body Resources is
        procedure Initialize (Object : in out Child) is
        begin
            if Fail then raise Program_Error; end if;
            Live := Live + 1;
        end Initialize;
        procedure Finalize (Object : in out Child) is
        begin
            Live := Live - 1;
        end Finalize;
    end Resources;
    use Resources;
    type Holder (Ref : access Child) is record
        Count : Integer := 3;
    end record;
    type Holder_Link is access all Holder;
    type Child_Link is access all Child;
    procedure Free is new Ada.Unchecked_Deallocation (Holder, Holder_Link);
    function Build return Holder_Link is
    begin
        return new Holder'(new Child, 4);
    end Build;
    function Make return Holder is
    begin
        return (new Child, 5);
    end Make;
    procedure Check (Condition : Boolean) is
    begin
        if not Condition then raise Constraint_Error; end if;
    end Check;
    procedure Consume (Object : access Child) is
    begin
        Check (Live = 1 and Object.Value = 7);
    end Consume;
    procedure Reject (Object : access Child) is
    begin
        raise Program_Error;
    end Reject;
    P : Holder_Link := Build;
begin
    Check (Live = 1 and P.Ref.Value = 7);
    Free (P);
    Check (Live = 0 and P = null);
    declare
        Object : Holder := Make;
        Escaped : Child_Link;
    begin
        Check (Live = 1 and Object.Ref.Value = 7 and Object.Count = 5);
        begin
            Escaped := Child_Link (Object.Ref);
            raise Constraint_Error;
        exception
            when Program_Error => Check (Escaped = null);
        end;
    end;
    Check (Live = 0);
    Consume (new Child);
    Check (Live = 0);
    begin
        Reject (new Child);
    exception
        when Program_Error => Check (Live = 0);
    end;
    Fail := True;
    begin
        P := Build;
        raise Constraint_Error;
    exception
        when Program_Error => Check (P = null and Live = 0);
    end;
    Put_Line ("anonymous coextension lifetimes passed");
end AnonymousCoextensions;
