procedure GenericPackagePartialErrors is
    generic
        type T is private;
        N : Integer := 3;
    package Model is
        subtype Element is T;
    private
        Secret : Integer := 7;
    end Model;
    package Data is new Model (Integer, 9);
    package Characters is new Model (Character, 9);
    generic
        with package P is new Model (T => Integer, N => <>);
    package Views is end Views;
    package Wrong_Type is new Views (Characters);
    generic
        with package P is new Model (T => <>, N => 3);
    package Numbers is end Numbers;
    package Wrong_Value is new Numbers (Data);
    generic
        with package P is new Model (T => <>);
    package Default_Required is end Default_Required;
    package Default_Mismatch is new Default_Required (Data);
    generic
        with package P is new Model (T => Integer, others => <>);
    package Hidden_Matched is
        Object : P.T;
    end Hidden_Matched;
    generic
        with package P is new Model (T => Integer, N => <>);
    package Hidden_Private is
        Value : Integer := P.Secret;
    end Hidden_Private;
    generic
        with package P is new Model (N => <>);
    package Missing_Type is end Missing_Type;
    generic
        with package P is new Model (T => Integer, Unknown => <>);
    package Bad_Name is end Bad_Name;
    generic
        with package P is new Model (T => Integer, 3);
    package Bad_Order is end Bad_Order;
    generic
        with package P is new Model (others => <>);
        with package Q is new Model (others => <>);
    procedure Distinct (Left : out P.T; Right : Q.T);
    procedure Distinct (Left : out P.T; Right : Q.T) is
    begin
        Left := Right;
    end Distinct;
    generic
        with package P is new Model (others => <>);
    package Forwarder is
        package Cannot_Constrain is new Views (P);
    end Forwarder;
    generic
        with package P is new Model (T => 3, others => <>);
    package Bad_Type_Actual is end Bad_Type_Actual;
    generic
        with function F (Value : Integer) return Integer;
        with function F (Value : Boolean) return Boolean;
    package Overloaded is end Overloaded;
    generic
        with package P is new Overloaded (F => <>, others => <>);
    package Ambiguous_Selector is end Ambiguous_Selector;
    generic
        with package P is new Overloaded (Integer'Succ, others => <>);
    package Hidden_Overload is
        Bad : Integer := P.F (1);
    end Hidden_Overload;
begin
    null;
end GenericPackagePartialErrors;
