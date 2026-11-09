package ConcurrentLibrary is
    type Result is limited private;
    function Make return Result;
    function Value (Object : Result) return Integer;
private
    task type Worker;
    protected type Guard is
        procedure Set (N : Integer);
        function Get return Integer;
    private
        N : Integer := 0;
    end Guard;
    type Result is limited record
        W : Worker;
        G : Guard;
    end record;
end ConcurrentLibrary;
