package body Protected_Model is
    protected body Counter is
        procedure Add (amount : Integer := 1) is
        begin
            value := value + amount;
        end Add;
        function Get return Integer is
        begin
            return value;
        end Get;
    end Counter;
end Protected_Model;
