with Ada.Text_IO;
with Ada.Exceptions;
with Ada.Unchecked_Deallocation;
procedure EntryExceptions is
    protected type Gate is
        entry Run (fail : Boolean; value : in out Integer);
        procedure NestedCall;
        function Get return Integer;
    private
        total : Integer := 0;
    end Gate;
    type Gate_Access is access Gate;
    alias : Gate_Access;
    protected body Gate is
        entry Run (fail : Boolean; value : in out Integer) when True is
        begin
            total := total + 1;
            value := 100;
            if fail then
                raise Constraint_Error with "entry body preserved";
            end if;
        end Run;
        procedure NestedCall is
            value : Integer := 0;
        begin
            alias.Run (False, value);
        end NestedCall;
        function Get return Integer is
        begin
            return total;
        end Get;
    end Gate;
    protected single is
        entry Check;
    end single;
    protected body single is
        entry Check when Check'Count = 0 is
        begin
            null;
        end Check;
    end single;
    protected counted is
        entry Check;
    end counted;
    protected body counted is
        entry Check when Check'Count > 0 is
        begin
            if Check'Count /= 0 then
                raise Program_Error;
            end if;
        end Check;
    end counted;
    procedure Free is new Ada.Unchecked_Deallocation (Gate, Gate_Access);
    object : Gate;
    value : Integer := 7;
    small : Integer range 0 .. 10 := 0;
begin
    declare
        type Boolean is range 0 .. 1;
        protected shadow is
            entry Check;
        end shadow;
        protected body shadow is
            entry Check when True is
            begin
                null;
            end Check;
        end shadow;
    begin
        shadow.Check;
    end;
    single.Check;
    counted.Check;
    begin
        object.Run (True, value);
        raise Program_Error;
    exception
        when e : Constraint_Error =>
            if value /= 7 or Ada.Exceptions.Exception_Message (e) /= "entry body preserved" then
                raise Program_Error;
            end if;
    end;
    begin
        object.Run (False, small);
        raise Program_Error;
    exception
        when Constraint_Error => null;
    end;
    object.Run (False, value);
    if object.Get /= 3 or value /= 100 then
        raise Program_Error;
    end if;
    alias := new Gate;
    begin
        object.NestedCall;
        raise Constraint_Error;
    exception
        when Program_Error => null;
    end;
    if alias.Get /= 0 then
        raise Program_Error;
    end if;
    alias.Run (False, value);
    Free (alias);
    begin
        alias.Run (False, value);
        raise Program_Error;
    exception
        when Constraint_Error => null;
    end;
    Ada.Text_IO.Put_Line ("entry exceptions passed");
end EntryExceptions;
