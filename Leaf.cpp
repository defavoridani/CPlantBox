
#include "Leaf.h"

/**
* Constructor
* This is a Copy Paste of the Root.cpp but it works independently, it has its own parameter file (in .stparam file) tropism, growth function, txt and vtp writing syleaf.
* All of those can be modified to fit the real growth of the Plant.
*
* Typically called by the Plant::Plant(), or Leaf::createNewLeaf().
* For leaf the initial node and node emergence time (netime) must be set from outside
*
* @param rs 			points to Leaf
* @param type 		    type of leaf that is created
* @param pheading		heading of parent leaf at emergence
* @param delay 		to give apical zone of parent time to develop
* @param parent		parent leaf
* @param pni			parent node index
* @param pbl			parent base length
*/
Leaf::Leaf(Plant* plant, Organ* parent, int type, double delay, Vector3d ilheading ,int pni, double pbl) :Organ(plant,parent,type,delay), pni(pni), pbl(pbl)
{
  initialLeafHeading=ilheading;
  std::cout << "Leaf pni = "<< pni<< std::endl;
//  std::cout << "Leaf constructor \n";
  LeafTypeParameter* ltp = (LeafTypeParameter*) plant->getOrganTypeParameter(Plant::ot_leafe, type);


  leaf_param = ltp->realize(); // throw the dice
  LeafParameter* leaf_p = (LeafParameter*) leaf_param;
  std::cout <<", "<<(LeafParameter*) leaf_param<< "\n";
  double beta = 2*M_PI*plant->rand(); // initial rotation
  Matrix3d ons = Matrix3d::ons(initialLeafHeading);
  ons.times(Matrix3d::rotX(beta));
  double theta = leaf_p->theta;
  if (parent->organType()!=Plant::ot_seed) { // scale if not a base leaf
    double scale = ltp->sa->getValue(parent->getNode(pni),this);
    theta*=scale;
  }
  ons.times(Matrix3d::rotZ(theta));
  // initial node
  if (parent->organType()!=Plant::ot_seed) { // the first node of the base leafs must be created in Seed::initialize()
    // otherwise, don't use addNode for the first node of the leaf,
    // since this node exists already and does not need a new identifier
    r_nodes.push_back(parent->getNode(pni));
    nodeIDs.push_back(parent->getNodeID(pni));
    nctimes.push_back(parent->getNodeCT(pni)+delay);
  }
}

int Leaf::organType() const
{
  return Plant::ot_leafe;
}

/**
* Simulates growth of this leaf for a time span dt
*
* @param dt       time step [day]
* @param silence  indicates if status messages are written to the console (cout) (default = false)
*/
void Leaf::simulate(double dt, bool silence)
{
  old_non = 0; // is set in Leaf:createSegments, the zero indicates the first call to createSegments

  const LeafParameter* lp = lParam(); // rename
  const LeafTypeParameter* ltp = ltParam();

  // increase age
  if (age+dt>lp->rlt) { // leaf life time
    dt=lp->rlt-age; // remaining life span
    alive = false; // this leaf is dead
  }
  age+=dt;

  if (alive) { // dead leaf wont grow
//createShootborneroot(silence);
    // probabilistic branching model (todo test)
    if ((age>0) && (age-dt<=0)) { // the leaf emerges in this time step
      double leaf_P = ltp->sbp->getValue(r_nodes.back(),this);
      if (leaf_P<1.) { // P==1 means the lateral emerges with probability 1 (default case)
        double leaf_p = 1.-std::pow((1.-leaf_P), dt); //probability of emergence in this time step
//        std::cout <<leaf_P<<", "<<leaf_p<< "\n";
        if (plant->rand()>leaf_p) { // not rand()<p
          age -= dt; // the leaf does not emerge in this time step
        }
      }
    }

    if (age>0) {

      // children first (lateral leafs grow even if base leaf is inactive)
      for (auto c:children) {
        c->simulate(dt,silence);
      }

      if (active) {

        // length increment
        double length_ = LeafgetLength(std::max(age-dt,0.)); // length of the leaf for unimpeded growth (i.e. length_==length for unimpeded growth)
        double targetlength = LeafgetLength(age);
        double e = targetlength-length_; //elongation in time step dt
        double scale = ltp->se->getValue(r_nodes.back(),this); // hope some of this is optimized out if not set
        double dl = std::max(scale*e, double(0)); // length increment, dt is not used anymore

        // create geometry
        if (lp->ln.size()>0) { // leaf has laterals
          // basal zone
          if ((dl>0)&&(length<lp->lb)) { // length is the current length of the leaf
            if (length+dl<=lp->lb) {
              createSegments(dl,silence);
              length+=dl;
              dl=0;
            } else {
              double ddx = lp->lb-length;
              createSegments(ddx,silence);
              dl-=ddx; // ddx already has been created
              length=lp->lb;
            }
          }
          // branching zone Condition will be changed later
          if ((dl>0)&&(length>=lp->lb)) {
            double s = lp->lb; // summed length
            for (size_t i=0; ((i<lp->ln.size()) && (dl>0)); i++) {
              s+=lp->ln.at(i);
              if (length<s) {
                if (i==children.size()) { // new lateral
                  createLateral(silence);

                }
                if (length+dl<=s) { // finish within inter-lateral distance i
                  createSegments(dl,silence);
                  length+=dl;
                  dl=0;
                } else { // grow over inter-lateral distance i
                  double ddx = s-length;
                  createSegments(ddx,silence);
                  dl-=ddx;
                  length=s;
                }
              }
            }
            if (dl>0) {
              if (lp->ln.size()==children.size()) { // new lateral (the last one)
                createLateral(silence);

              }
            }
          }
          // apical zone
          if (dl>0) {
            createSegments(dl,silence);
            length+=dl;
          }
        } else { // no laterals
          if (dl>0) {
            createSegments(dl,silence);
            length+=dl;
          }
        } // if laterals
      } // if active
      active = LeafgetLength(std::max(age,0.))<(lp->getK()-dx()/10); // become inactive, if final length is nearly reached
    }
  } // if alive



}

/**
*
*/
double Leaf::getScalar(int stype) const {
  switch(stype) {
  // st_rlt, st_meanln, st_stdln , st_nob, st_surface, , // leaf level
  case Plant::st_lb:
    return lParam()->lb;
  case Plant::st_la:
    return lParam()->la;
  case Plant::st_r:
    return lParam()->r;
  case Plant::st_radius:
    return lParam()->a;
  case Plant::st_theta:
    return lParam()->theta;
  case Plant::st_rlt:
    return lParam()->rlt;
  case Plant::st_meanln:
    return std::accumulate(lParam()->ln.begin(), lParam()->ln.end(), 0.0) / lParam()->ln.size();
  case Plant::st_stdln: {
    const std::vector<double>& v_ = lParam()->ln;
    double mean = std::accumulate(v_.begin(), v_.end(), 0.0) / v_.size();
    double sq_sum = std::inner_product(v_.begin(), v_.end(), v_.begin(), 0.0);
    return std::sqrt(sq_sum / v_.size() - mean * mean);
  }
  case Plant::st_surface:
    return lParam()->a*lParam()->a*M_PI*length;
  case Plant::st_nob:
    return lParam()->ln.size();
  default:
    return  Organ::getScalar(stype);
  }
}

/**
* Analytical creation (=emergence) time of a node at a length along the leaf
*
* @param length   length of the leaf [cm]
*/
double Leaf::getCreationTime(double length)
{
  assert(length>=0);
  double leafage = LeafgetAge(length);
  assert(leafage>=0);
  if (parent->organType()!=Plant::ot_stem) {
    if (parent->organType()==Plant::ot_shoot) {
      double pl = pbl+((Leaf*)parent)->ltParam()->la; // parent length, when this leaf was created
      double pAge=((Leaf*)parent)->getCreationTime(pl);
      return leafage+pAge;
    } else { // organ type is seed
      return leafage;
    }
  } else {
    return leafage;
  }
}

/**
* Analytical length of the leaf at a given age
*
* @param age          age of the leaf [day]
*/
double Leaf::LeafgetLength(double age)
{
  assert(age>=0);
  return ltParam()->growth->LeafgetLength(age,ltParam()->r,ltParam()->getK(),this);
}

/**
* Analytical age of the leaf at a given length
*
* @param length   length of the leaf [cm]
*/
double Leaf::LeafgetAge(double length)
{
  assert(length>=0);
  return ltParam()->growth->LeafgetAge(length,ltParam()->r,ltParam()->getK(),this);
}

/**
*
*/
LeafTypeParameter* Leaf::ltParam() const {
  return (LeafTypeParameter*)getOrganTypeParameter();
}

/**
*
*/
double Leaf::dx() const
{
  return ((LeafTypeParameter*)getOrganTypeParameter())->dx;
}

/**
* Creates a new lateral by calling Leaf::createNewleaf().
*
* Overwrite this method to implement more specialized leaf classes.
*/
void Leaf::createLateral(bool silence)
{
  const LeafParameter* lp = lParam(); // rename
  int lt = ltParam()->getLateralType(r_nodes.back());
  	std::cout << "createLateral()\n";
  	std::cout << "lateral type " << lt << "\n";

  if (lt>0) {
    double ageLN = this->LeafgetAge(length); // age of leaf when lateral node is created
    double ageLG = this->LeafgetAge(length+lp->la); // age of the leaf, when the lateral starts growing (i.e when the apical zone is developed)
    double delay = ageLG-ageLN; // time the lateral has to wait
    Vector3d h = heading(); // current heading
    Leaf* lateral = new Leaf(plant, this, lt, delay, h,  r_nodes.size()-1, length);
    children.push_back(lateral);
    lateral->simulate(age-ageLN,silence); // pass time overhead (age we want to achieve minus current age)
    }


}



/**
*  Creates nodes and node emergence times for length l,
*  and updates the leaf heading
*
*  Cecks that each new segments length is <= dx but >= ddx
*
*  @param l       length the leaf growth [cm]
*/
void Leaf::createSegments(double l, bool silence)
{
  // std::cout << "createSegments("<< l << ")\n";
  assert(l>0);
  double sl=0; // summed length of created segment

  // shift first node to axial resolution
  int nn = r_nodes.size();
  if (old_non==0) { // first call of createSegments (in leaf::simulate)
    if (nn>1) {
      auto n2 = r_nodes.at(nn-2);
      auto n1 = r_nodes.at(nn-1);
      double olddx = n1.minus(n2).length();
      if (olddx<dx()*0.99) { // shift node instead of creating a new node

        Vector3d h; // current heading
        if (nn>2) {
          h = n2.minus(r_nodes.at(nn-3));
          h.normalize();
        } else {
          h = initialLeafHeading;
        }
        double sdx = std::min(dx()-olddx,l);

        Matrix3d ons = Matrix3d::ons(h);
        Vector2d ab = ltParam()->tropism->getHeading(r_nodes.at(nn-2),ons,olddx+sdx,this);
        ons.times(Matrix3d::rotX(ab.y));
        ons.times(Matrix3d::rotZ(ab.x));
        Vector3d newdx = Vector3d(ons.column(0).times(olddx+sdx));

        Vector3d newnode = Vector3d(r_nodes.at(nn-2).plus(newdx));
        sl = sdx;
        double ct = this->getCreationTime(length+sl);
        r_nodes[nn-1] = newnode;
        nctimes[nn-1] = std::max(ct,plant->getSimTime()); // in case of impeded growth the node emergence time is not exact anymore, but might break down to temporal resolution
        old_non = nn-1;
        l -= sdx;
        if (l<=0) { // ==0 should be enough
          return;
        }
      }
    }
    old_non = nn; // CHECK
  }

  if (l<smallDx) {
    if (!silence) {
      std::cout << "skipped small segment (<"<< smallDx << ") \n";
    }
    return;
  }

  int n = floor(l/dx());
  // create n+1 new nodes
  for (int i=0; i<n+1; i++) {

    double sdx; // segment length (<=dx)
    if (i<n) {  // normal case
      sdx = dx();
    } else { // last segment
      sdx = l-n*dx();
      if (sdx<smallDx) {
        if (!silence) {
          std::cout << "skipped small segment (<"<< smallDx << ") \n";
        }
        return;
      }
    }
    sl+=sdx;

    Vector3d h= heading();
    Matrix3d ons = Matrix3d::ons(h);
    Vector2d ab = ltParam()->tropism->getHeading(r_nodes.back(),ons,sdx,this);
    ons.times(Matrix3d::rotX(ab.y));
    ons.times(Matrix3d::rotZ(ab.x));
    Vector3d newdx = Vector3d(ons.column(0).times(sdx));
    Vector3d newnode = Vector3d(r_nodes.back().plus(newdx));
    double ct = this->getCreationTime(length+sl);
    ct = std::max(ct,plant->getSimTime()); // in case of impeded growth the node emergence time is not exact anymore, but might break down to temporal resolution
    // std::cout<<"add node "<<newnode.toString()<<"\n";
    addNode(newnode,ct);

  } // for

}
//***********************leaf heading*****************************
Vector3d Leaf::heading() const {
  Vector3d h;
  if ((leaf_param->subType == 1) || (this->r_nodes.size()<=1) ) {// Make heading upward if it is main leaf and
    h = initialLeafHeading;// getHeading(b-a)
  } else {
     h = r_nodes.back().minus(r_nodes.at(r_nodes.size()-2));
  }
  return h;
}

/**
* Adds the next node to the leaf.
*
* Add nodes only with this function! For simplicity nodes can not be deleted, and leafs can only become deactivated by dying
*
* @param n        the new node
* @param t        exact creation time of the node
*/
void Leaf::addNode(Vector3d n, double t)
{
  assert(t>=0.);
  r_nodes.push_back(n); // node
  nodeIDs.push_back(plant->getNodeIndex()); // new unique id
  nctimes.push_back(t); // exact creation time
}

/**
* writes RSML leaf tag
*
* @param cout      typically a file out stream
* @param indent    we care for looks
*/
void Leaf::writeRSML(std::ostream & cout, std::string indent) const
{
  if (this->r_nodes.size()>1) {
    cout << indent << "<leaf id=\"" <<  id << "\">\n";  // open stem

      /* geometry tag */
      cout << indent << "\t<geometry>\n"; // open geometry
      cout << indent << "\t\t<polyline>\n"; // open polyline
      // polyline nodes
      cout << indent << "\t\t\t" << "<point ";
      Vector3d v = r_nodes.at(0);
      cout << "x=\"" << v.x << "\" y=\"" << v.y << "\" z=\"" << v.z << "\"/>\n";
      int n = this->plant->rsmlReduction;
      for (size_t i = 1; i<r_nodes.size()-1; i+=n) {
        cout << indent << "\t\t\t" << "<point ";
        Vector3d v = r_nodes.at(i);
        cout << "x=\"" << v.x << "\" y=\"" << v.y << "\" z=\"" << v.z << "\"/>\n";
      }
      cout << indent << "\t\t\t" << "<point ";
      v = r_nodes.at(r_nodes.size()-1);
      cout << "x=\"" << v.x << "\" y=\"" << v.y << "\" z=\"" << v.z << "\"/>\n";
      cout << indent << "\t\t</polyline>\n"; // close polyline
      cout << indent << "\t</geometry>\n"; // close geometry

      /* properties */
      cout << indent <<"\t<properties>\n"; // open properties
      // TODO
      cout << indent << "\t</properties>\n"; // close properties

      cout << indent << "\t<functions>\n"; // open functions
      cout << indent << "\t\t<function name='emergence_time' domain='polyline'>\n"; // open functions
      cout << indent << "\t\t\t" << "<sample>" << nctimes.at(0) << "</sample>\n";
      for (size_t i = 1; i<nctimes.size()-1; i+=n) {
        cout << indent << "\t\t\t" << "<sample>" << nctimes.at(i) << "</sample>\n";

      }
      cout << indent << "\t\t\t" << "<sample>" << nctimes.at(nctimes.size()-1) << "</sample>\n";

      cout << indent << "\t\t</function>\n"; // close functions
      cout << indent << "\t</functions>\n"; // close functions

      /* laterals leafs */
      for (size_t i = 0; i<children.size(); i++) {
        children[i]->writeRSML(cout,indent+"\t");
      }

      cout << indent << "</root>\n"; // close leaf
  }
}

/**
 * Quick info about the object for debugging
 */
std::string Leaf::toString() const
{
  std::stringstream str;
  str << "Root #"<< id <<": type "<<leaf_param->subType << ", length: "<< length << ", age: " <<age<<" with "<< children.size() << " laterals\n";
  return str.str();
}

