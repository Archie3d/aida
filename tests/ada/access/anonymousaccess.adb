with Ada.Text_IO; use Ada.Text_IO;
procedure AnonymousAccess is
    type Link is access all Integer;
    Global : aliased Integer := 1;
    Saved : Link;
    P : access Integer := Global'Access;
    procedure Use_It (X : access Integer) is
    begin
        X.all := X.all + 1;
        Saved := Link (X);
    end Use_It;
    procedure Forward (X : access Integer) is
    begin
        Use_It (X);
    end Forward;
    procedure Run (Action : access procedure (X : in out Integer)) is
    begin
        Action (Global);
    end Run;
    function Extended (X : access Integer) return access Integer is
    begin
        return Result : access Integer := X do
            Result.all := Result.all + 1;
            return;
        end return;
    end Extended;
    procedure Callback_Result (Action : access function return access Integer) is
    begin
        Saved := Link (Action.all);
    end Callback_Result;
    generic
    package Template is
        procedure Add (P : access Integer);
    end Template;
    package body Template is
        procedure Add (P : access Integer) is
        begin
            P.all := P.all + 1;
        end Add;
    end Template;
    package Instance is new Template;
    procedure Nested is
        Bias : Integer := 10;
        Local : aliased Integer := 0;
        function Local_Result return access Integer is
        begin
            return Local'Access;
        end Local_Result;
        procedure Add (X : in out Integer) is
        begin
            X := X + Bias;
        end Add;
    begin
        Run (Add'Access);
        begin
            Callback_Result (Local_Result'Access);
            raise Constraint_Error;
        exception
            when Program_Error => null;
        end;
    end Nested;
begin
    Use_It (P);
    Nested;
    P := Extended (Global'Access);
    begin
        Run (null);
        raise Program_Error;
    exception
        when Constraint_Error => null;
    end;
    declare
        Local : aliased Integer := 0;
        package Deep is new Template;
    begin
        Instance.Add (Local'Access);
        Deep.Add (Local'Access);
        if Local /= 2 then raise Program_Error; end if;
    end;
    Put_Line (Integer'Image (Global));
    declare
        Local : aliased Integer := 8;
    begin
        begin
            Forward (Local'Access);
            raise Constraint_Error;
        exception
            when Program_Error => Put_Line ("caught");
        end;
    end;
end AnonymousAccess;
