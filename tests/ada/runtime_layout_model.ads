package Runtime_Layout_Model is
    N : Integer := 3;
    subtype Text is String (2 .. N);
    type Holder is record
        First : Integer := 11;
        Data : Text := "ab";
        Last : Long_Integer := 99;
    end record;
    type Rows is array (1 .. 2) of Holder;
    Global : Holder;
    Table : Rows;
    protected type Counter is
        procedure Increment;
        function Value return Integer;
    private
        Data : Text := "ab";
        Count : Integer := 0;
    end Counter;
    Counted : Counter;
    type Pointer is access String;
    Heap : Pointer := new String (2 .. N);
    generic
        Count : Positive;
    package Buffer is
        type Item is record
            Data : String (1 .. Count) := (others => 'b');
            Last : Integer := Count;
        end record;
        Value : Item;
    end Buffer;
    package Three is new Buffer (N);
    type Hidden is private;
    function Make return Holder;
    function Make_Hidden return Hidden;
    procedure Check_Hidden (X : Hidden);
private
    type Hidden is record
        Data : String (1 .. N) := (others => 'h');
        Last : Integer := 42;
    end record;
    Hidden_Value : Hidden;
end Runtime_Layout_Model;
