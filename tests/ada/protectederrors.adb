procedure ProtectedErrors is
    procedure SetValue (value : out Integer) is
    begin
        value := 4;
    end SetValue;
    protected type Counter is
        procedure Change;
        function Bad return Integer;
        function BadMode (value : out Integer) return Integer;
    private
        value : Integer := 0;
        procedure Hidden;
    end Counter;
    protected body Counter is
        procedure Change is
        begin
            value := 1;
        end Change;
        function Bad return Integer is
            aliasValue : Integer renames value;
            procedure Nested is
            begin
                value := 3;
            end Nested;
        begin
            aliasValue := 2;
            Counter.value := 3;
            SetValue (value);
            Change;
            Nested;
            return value;
        end Bad;
        function BadMode (value : out Integer) return Integer is
        begin
            return 0;
        end BadMode;
        procedure Hidden is
        begin
            null;
        end Hidden;
    end Counter;
    left, right : Counter;
    copy : Counter := left;
    function Result return Counter;
    function Result return Counter is
    begin
        return left;
    end Result;
    procedure AliasChange renames left.Change;
    type Callback is access procedure;
    callbackObject : Callback := left.Change'Access;
    generic
        with procedure Operation;
    procedure Wrapper;
    procedure Wrapper is
    begin
        Operation;
    end Wrapper;
    procedure Instance is new Wrapper (left.Change);
    protected type Missing is
        procedure Operation;
    end Missing;
begin
    left := right;
    if left = right then
        null;
    end if;
    left.value := 1;
    left.Hidden;
    Counter.Change;
end ProtectedErrors;
