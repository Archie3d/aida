with Ada.Text_IO; use Ada.Text_IO;
with Ada.Finalization;
procedure Forwardelaboration is
    Live : Integer := 0;
    package Cleanup is
        type Guard is new Ada.Finalization.Limited_Controlled with null record;
        overriding procedure Initialize (Object : in out Guard);
        overriding procedure Finalize (Object : in out Guard);
    end Cleanup;
    package body Cleanup is
        procedure Initialize (Object : in out Guard) is
        begin
            Live := Live + 1;
        end Initialize;
        procedure Finalize (Object : in out Guard) is
        begin
            Live := Live - 1;
        end Finalize;
    end Cleanup;
    procedure Trial is
        generic
            Value : Integer;
        function G return Integer;
        function Invoke return Integer is
            Object : Cleanup.Guard;
            function Instance is new G (2);
        begin
            return Instance;
        exception
            when Program_Error => raise Constraint_Error;
        end Invoke;
        function Recover return Integer is
        begin
            return Invoke;
        exception
            when Program_Error =>
                if Live /= 0 then
                    raise Constraint_Error;
                end if;
                Put_Line ("early subprogram instance cleaned");
                return 0;
        end Recover;
        Early : Integer := Recover;
        Bias : Integer := 40;
        function G return Integer is
        begin
            return Bias + Value;
        end G;
        generic
            Value : Integer;
        package Template is
            type Number is new Integer;
            function Get return Number;
        end Template;
        function Package_Invoke return Integer is
            Object : Cleanup.Guard;
            package Instance is new Template (3);
            X : Instance.Number := Instance.Get;
        begin
            return Integer (X);
        exception
            when Program_Error => raise Constraint_Error;
        end Package_Invoke;
        function Package_Recover return Integer is
        begin
            return Package_Invoke;
        exception
            when Program_Error =>
                if Live /= 0 then
                    raise Constraint_Error;
                end if;
                Put_Line ("early package instance cleaned");
                return 0;
        end Package_Recover;
        Package_Early : Integer := Package_Recover;
        package body Template is
            function Get return Number is
            begin
                return Number (Bias + Value);
            end Get;
        end Template;
        Changes : Integer := 0;
        function Bump return Integer is
        begin
            Changes := Changes + 1;
            return Changes;
        end Bump;
        generic
        package Optional_Body is
            Seen : Integer := Bump;
        end Optional_Body;
        function Optional_Invoke return Integer is
            package Instance is new Optional_Body;
        begin
            return Instance.Seen;
        end Optional_Invoke;
        function Optional_Recover return Integer is
        begin
            return Optional_Invoke;
        exception
            when Program_Error => return 0;
        end Optional_Recover;
        Optional_Early : Integer := Optional_Recover;
        package body Optional_Body is
        begin
            Changes := Changes + 10;
        end Optional_Body;
        generic
        package No_Body is
            Value : Integer := 9;
        end No_Body;
        package Plain is new No_Body;
    begin
        if Optional_Early /= 0 or Changes /= 0 or Optional_Invoke /= 1 or Changes /= 11 or Plain.Value /= 9 then
            raise Constraint_Error;
        end if;
        if Early /= 0 or Package_Early /= 0 or Invoke /= 42 or Package_Invoke /= 43 or Live /= 0 then
            raise Constraint_Error;
        end if;
        Put_Line ("forward instances ready");
    end Trial;
begin
    Trial;
    Trial;
end Forwardelaboration;
