with Ada.Finalization;
with Ada.Exceptions; use Ada.Exceptions;
with Ada.Text_IO; use Ada.Text_IO;
procedure ControlledExceptions is
    Live : Integer := 0;
    Fail_Init : Boolean := False;
    Fail_Finalize : Boolean := False;
    type Guard is new Ada.Finalization.Controlled with null record;
    overriding procedure Initialize (Object : in out Guard);
    overriding procedure Finalize (Object : in out Guard);
    procedure Initialize (Object : in out Guard) is
    begin
        if Fail_Init then
            raise Constraint_Error with "initialization failed";
        end if;
        Live := Live + 1;
    end Initialize;
    procedure Finalize (Object : in out Guard) is
    begin
        -- Calls and handled exceptions must work even during unwinding.
        begin
            raise Storage_Error;
        exception
            when Storage_Error => null;
        end;
        Live := Live - 1;
        Put_Line ("remaining" & Integer'Image (Live));
        if Fail_Finalize then
            raise Constraint_Error with "finalization failed";
        end if;
    end Finalize;

    procedure Failed_Declaration is
        A : Guard;
        function Fail return Boolean is
        begin
            Fail_Init := True;
            return True;
        end Fail;
        Dummy : Boolean := Fail;
        B : Guard;
    begin
        raise Program_Error;
    exception
        when others => Put_Line ("wrong declaration handler");
    end Failed_Declaration;

    procedure Failed_Return is
        A, B : Guard;
    begin
        return;
    exception
        when others => Put_Line ("wrong return handler");
    end Failed_Return;
begin
    begin
        Failed_Declaration;
    exception
        when E : Constraint_Error =>
            if Live /= 0 or Exception_Message (E) /= "initialization failed" then
                raise Program_Error;
            end if;
            Put_Line ("initialization failure");
    end;
    Fail_Init := False;
    declare
        A : Guard;
    begin
        declare
            B : Guard;
        begin
            raise Constraint_Error with "original occurrence";
        end;
    exception
        when E : Constraint_Error =>
            if Live /= 1 or Exception_Message (E) /= "original occurrence" then
                raise Program_Error;
            end if;
            Put_Line ("handler retains enclosing object");
    end;
    Fail_Finalize := True;
    begin
        declare
            A, B : Guard;
        begin
            null;
        exception
            when others => Put_Line ("wrong finalization handler");
        end;
    exception
        when Program_Error => Put_Line ("normal finalization failure");
    end;
    begin
        Failed_Return;
    exception
        when Program_Error => Put_Line ("return finalization failure");
    end;
    begin
        declare
            A, B : Guard;
        begin
            raise Storage_Error;
        end;
    exception
        when Program_Error => Put_Line ("unwinding finalization failure");
    end;
    begin
        loop
            declare
                A, B : Guard;
            begin
                exit;
            exception
                when others => Put_Line ("wrong exit handler");
            end;
        end loop;
    exception
        when Program_Error => Put_Line ("exit finalization failure");
    end;
    if Live /= 0 then
        raise Constraint_Error;
    end if;
    Put_Line ("exceptions passed");
end ControlledExceptions;
