procedure EntryErrors is
    protected type Gate is
        entry Wait (value : Integer);
        entry Missing;
        procedure Wrong;
        procedure Internal;
        function BadCount return Integer;
    private
        entry Hidden;
    end Gate;
    protected body Gate is
        entry Wait (value : Integer) when value > 0 is
        begin
            null;
        end Wait;
        entry Wrong when True is
        begin
            null;
        end Wrong;
        entry Orphan when True is
        begin
            null;
        end Orphan;
        procedure Internal is
        begin
            Wait (1);
        end Internal;
        function BadCount return Integer is
        begin
            return Wrong'Count;
        end BadCount;
        entry Hidden when True is
        begin
            null;
        end Hidden;
    end Gate;
    object : Gate;
    count : Integer := object.Wait'Count;
begin
    object.Hidden;
end EntryErrors;
