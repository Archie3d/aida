procedure GenericPackageErrors is
    generic
        type T is private;
    package Model is
        subtype Element is T;
    private
        Secret : Integer := 7;
    end Model;
    generic
        type T is private;
    package Different is
        subtype Element is T;
    end Different;
    package Data is new Model (Integer);
    package Other is new Different (Integer);
    package Ordinary is end Ordinary;
    generic
        with package P is new Model (<>);
    package Views is end Views;
    package Wrong_Template is new Views (Other);
    package Not_Instance is new Views (Ordinary);
    package Not_Package is new Views (Integer);
    package Missing is new Views;
    generic
        with package P is new Model (Integer);
    package Exact is end Exact;
    package Characters is new Model (Character);
    package Wrong_Type is new Exact (Characters);
    generic
        with package P is new Model (<>);
        with package Q is new Model (<>);
    procedure Distinct (Left : out P.Element; Right : Q.Element);
    procedure Distinct (Left : out P.Element; Right : Q.Element) is
    begin
        Left := Right;
    end Distinct;
    generic
        with package P is new Model (<>);
    package Privacy is
        Exposed : Integer := P.Secret;
    end Privacy;
    generic
        with package P is new Model (Integer);
    package Matched_Formals_Hidden is
        Object : P.T;
    end Matched_Formals_Hidden;
    generic
        with package P is new Ordinary (<>);
    package Bad_Template is end Bad_Template;
    generic
        with package P is new Model;
    package Missing_Contract_Actual is end Missing_Contract_Actual;
begin
    null;
end GenericPackageErrors;
