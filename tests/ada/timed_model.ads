package Timed_Model is
    pragma Elaborate_Body;
    protected Notice is
        entry Wait;
        procedure Signal;
    private
        ready : Boolean := False;
    end Notice;
    task Pulse;
end Timed_Model;
