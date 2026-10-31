package Protected_Model is
    protected type Counter is
        procedure Add (amount : Integer := 1);
        function Get return Integer;
    private
        value : Integer := 0;
    end Counter;
    shared : Counter;
end Protected_Model;
