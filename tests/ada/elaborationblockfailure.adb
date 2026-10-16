with Ada.Finalization;
with Ada.Text_IO; use Ada.Text_IO;
package Block_Failure is
    Failed : exception;
end Block_Failure;
package body Block_Failure is
begin
    declare
        Value : Integer := 42;
        type Guard is new Ada.Finalization.Controlled with null record;
        overriding procedure Finalize (Object : in out Guard) is
        begin
            Put_Line ("block finalized" & Integer'Image (Value));
        end Finalize;
        Object : Guard;
        package Local is
        end Local;
        package body Local is
        begin
            raise Failed;
        exception
            when Failed =>
                Put_Line ("local package handler");
                raise;
        end Local;
    begin
        Put_Line ("unreachable block");
    exception
        when Failed => Put_Line ("unreachable declaration handler");
    end;
end Block_Failure;
with Block_Failure;
with Ada.Text_IO; use Ada.Text_IO;
procedure ElaborationBlockFailure is
begin
    Put_Line ("unreachable main");
end ElaborationBlockFailure;
