package body Entry_Model is
    protected body Cell is
        entry Put (value : Integer) when not full is
        begin
            data := value;
            full := True;
        end Put;
        entry Get (value : out Integer) when full is
        begin
            value := data;
            full := False;
        end Get;
    end Cell;
end Entry_Model;
