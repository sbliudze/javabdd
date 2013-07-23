// IteratorTests.java, created Oct 19, 2004 1:16:36 AM by joewhaley
// Copyright (C) 2004 John Whaley <jwhaley@alum.mit.edu>
// Licensed under the terms of the GNU LGPL; see COPYING for details.
package bdd;

import java.util.HashSet;
import java.util.Iterator;
import java.util.Random;
import java.util.Set;
import junit.framework.Assert;
import net.sf.javabdd.BDD;
import net.sf.javabdd.BDDDomain;
import net.sf.javabdd.BDDFactory;

/**
 * IteratorTests
 * 
 * @author jwhaley
 * @version $Id: IteratorTests.java,v 1.1 2004/10/19 11:46:29 joewhaley Exp $
 */
public class IteratorTests extends BDDTestCase {
    public static void main(String[] args) {
        junit.textui.TestRunner.run(IteratorTests.class);
    }
    
    public void testIterator() {
        reset();
        Assert.assertTrue(hasNext());
        while (hasNext()) {
            BDDFactory bdd = nextFactory();
            bdd.setNodeTableSize(200000);
            int domainSize = 1024;
            bdd.extDomain(new int[] { domainSize, domainSize });
            BDDDomain d = bdd.getDomain(0);
            BDDDomain d2 = bdd.getDomain(1);
            Random r = new Random();
            int times = 1000;
            int combine = 400;
            for (int i = 0; i < times; ++i) {
                int count = r.nextInt(combine);
                BDD b = bdd.zero();
                for (int j = 0; j < count; ++j) {
                    int varNum = r.nextInt(domainSize);
                    BDD c = d.ithVar(varNum); //.andWith(d2.ithVar(domainSize - varNum - 1));
                    b.orWith(c);
                }
                BDD var = d.set();
                Iterator i1 = b.iterator(var);
                Iterator i2 = b.iterator2(var);
                b.free();
                Set s1 = new HashSet();
                Set s2 = new HashSet();
                while (i1.hasNext()) {
                    BDD b1 = (BDD) i1.next();
                    double sc = b1.satCount(var); 
                    Assert.assertEquals(1., sc, 0.0000001);
                    s1.add(b1);
                }
                while (i2.hasNext()) {
                    BDD b2 = (BDD) i2.next();
                    double sc = b2.satCount(var); 
                    Assert.assertEquals(1., sc, 0.0000001);
                    s2.add(b2);
                }
                var.free();
                if (!s1.equals(s2)) {
                    Set s1_minus_s2 = new HashSet(s1);
                    s1_minus_s2.removeAll(s2);
                    Set s2_minus_s1 = new HashSet(s2);
                    s2_minus_s1.removeAll(s1);
                    Assert.fail("iterator() contains these extras: "+s1_minus_s2+"\n"+
                        "iterator2() contains these extras: "+s2_minus_s1);
                }
                for (Iterator k = s1.iterator(); k.hasNext(); ) {
                    BDD q = (BDD) k.next();
                    q.free();
                }
                for (Iterator k = s2.iterator(); k.hasNext(); ) {
                    BDD q = (BDD) k.next();
                    q.free();
                }
            }
        }
    }
}
