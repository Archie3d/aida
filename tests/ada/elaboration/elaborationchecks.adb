with Ada.Text_IO; use Ada.Text_IO;
with Ada.Finalization;
procedure Elaborationchecks is
    Live : Integer := 0;
    package Model is
        type Guard is new Ada.Finalization.Limited_Controlled with null record;
        overriding procedure Initialize (Object : in out Guard);
        overriding procedure Finalize (Object : in out Guard);
    end Model;
    package body Model is
        procedure Initialize (Object : in out Guard) is
        begin
            Live := Live + 1;
        end Initialize;
        procedure Finalize (Object : in out Guard) is
        begin
            Live := Live - 1;
        end Finalize;
    end Model;
    procedure Trial is
        function Later return Integer;
        type Callback is access function return Integer;
        F : Callback := Later'Access;
        function Probe return Integer is
            Object : Model.Guard;
        begin
            return F.all;
        end Probe;
        procedure Direct is
        begin
            Put_Line (Integer'Image (Later));
            raise Constraint_Error;
        exception
            when Program_Error => Put_Line ("direct call checked");
        end Direct;
        function Recover return Integer is
        begin
            Direct;
            return Probe;
        exception
            when Program_Error =>
                if Live /= 0 then
                    raise Constraint_Error;
                end if;
                Put_Line ("callback checked and finalized");
                return 0;
        end Recover;
        X : Integer := Recover;
        function Later return Integer is
        begin
            return 42;
        end Later;
        function Alias return Integer;
        function Renaming_Probe return Integer is
        begin
            return Alias;
        exception
            when Program_Error => Put_Line ("renaming checked"); return 0;
        end Renaming_Probe;
        Y : Integer := Renaming_Probe;
        function Alias return Integer renames Later;
        generic
        function Generic_Value return Integer;
        function Generic_Value return Integer is
        begin
            return Later;
        end Generic_Value;
        function Instance is new Generic_Value;
    begin
        if F.all /= 42 or Alias /= 42 or Instance /= 42 then
            raise Constraint_Error;
        end if;
        Put_Line ("elaborated calls ok");
    end Trial;
begin
    Trial;
    Trial;
    for I in 1 .. 2 loop
        declare
            function Later return Integer;
            function Probe return Integer is
            begin
                return Later;
            exception
                when Program_Error => return 0;
            end Probe;
            Early : Integer := Probe;
            function Later return Integer is
            begin
                return 1;
            end Later;
        begin
            if Early /= 0 or Later /= 1 then
                raise Constraint_Error;
            end if;
        end;
    end loop;
    Put_Line ("block flags reset");
end Elaborationchecks;
