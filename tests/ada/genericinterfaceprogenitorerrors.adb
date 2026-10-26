procedure GenericInterfaceProgenitorErrors is
    type I is interface;
    type L is limited interface;
    type S is synchronized interface;
    type Record_Type is abstract tagged null record;
    generic
        type F is interface and Record_Type;
    package Wrong_Progenitor is
    end Wrong_Progenitor;
    generic
        type F is interface and I and I;
    package Duplicate_Progenitor is
    end Duplicate_Progenitor;
    generic
        type F is limited interface and I;
    package Nonlimited_Ancestor is
    end Nonlimited_Ancestor;
    generic
        type F is limited interface and S;
    package Synchronized_Ancestor is
    end Synchronized_Ancestor;
    generic
        type F is new L and I with private;
    package Mixed_Ancestors is
    end Mixed_Ancestors;
    generic
        type F is limited new L and I with private;
    package Limited_Mixed_Ancestors is
    end Limited_Mixed_Ancestors;
    generic
        type F is synchronized interface and S;
    package Good_Synchronized is
    end Good_Synchronized;
begin
    null;
end GenericInterfaceProgenitorErrors;
