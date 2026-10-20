package Entry_Model is
    protected type Cell is
        entry Put (value : Integer);
        entry Get (value : out Integer);
    private
        full : Boolean := False;
        data : Integer := 0;
    end Cell;
    shared : Cell;
end Entry_Model;
