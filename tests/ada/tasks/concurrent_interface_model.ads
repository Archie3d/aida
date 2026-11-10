package Concurrent_Interface_Model is
    type Contract is synchronized interface;
    procedure Add (Self : in out Contract; Value : in out Integer) is abstract;
    type Task_Contract is task interface and Contract;
    type Protected_Contract is protected interface and Contract;
    task type Worker (Seed : Positive := 4) is new Task_Contract with
        overriding entry Add (Value : in out Integer);
    end Worker;
    protected type Counter (Size : Positive := 2) is new Protected_Contract with
        overriding procedure Add (Value : in out Integer);
    private
        Data : String (1 .. Size) := (others => 'x');
    end Counter;
    generic
        type Actual (<>) is synchronized new Contract with private;
    procedure Apply (Self : in out Actual; Value : in out Integer);
end Concurrent_Interface_Model;
