procedure GenericSynchronizedErrors is
    type Ordinary is interface;
    type Base is limited interface;
    type Sync is synchronized interface and Base;
    type Task_View is task interface and Sync;
    type Protected_View is protected interface and Sync;
    type Plain is limited new Base with null record;
    generic
        type T is synchronized new Integer;
    package Untagged is end Untagged;
    generic
        type T is abstract synchronized new Ordinary with private;
    package Nonlimited is end Nonlimited;
    generic
        type T is abstract synchronized new Base and Ordinary with private;
    package Nonlimited_Progenitor is end Nonlimited_Progenitor;
    generic
        type T is abstract limited new Sync with private;
    package Missing_Keyword is end Missing_Keyword;
    generic
        type T is abstract new Base and Sync with private;
    package Missing_Progenitor_Keyword is end Missing_Progenitor_Keyword;
    generic
        type T is abstract synchronized new Task_View and Protected_View with private;
    package Conflicting is end Conflicting;
    generic
        type T is abstract synchronized new Base and Ordinary'Class with private;
    package Class_Progenitor is end Class_Progenitor;
    generic
        type T is abstract synchronized new Base with private;
    package View is end View;
    package Wrong_Actual is new View (Plain);
    package Limited_Interface is new View (Base);
    generic
        type T is abstract synchronized new Task_View with private;
    package Task_Only is end Task_Only;
    package Wrong_Category is new Task_Only (Protected_View);
    generic
        type T is abstract synchronized new Base with private;
    package Cannot_Implement is
        type Child is new T with null record;
    end Cannot_Implement;
    generic
        type T is synchronized new Sync with private;
    package Concrete_Formal is end Concrete_Formal;
    package Abstract_Actual is new Concrete_Formal (Sync);
begin
    null;
end GenericSynchronizedErrors;
