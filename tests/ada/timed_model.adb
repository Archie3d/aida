with Ada.Real_Time; use Ada.Real_Time;
package body Timed_Model is
    protected body Notice is
        entry Wait when ready is
        begin
            null;
        end Wait;
        procedure Signal is
        begin
            ready := True;
        end Signal;
    end Notice;
    task body Pulse is
        deadline : Time := Clock + Milliseconds (2);
    begin
        delay until deadline;
        Notice.Signal;
    end Pulse;
begin
    -- Delay statements also run during environment-task elaboration.
    delay 0.001;
end Timed_Model;
