#include "Plant.h"

const std::vector<std::string> Plant::scalarTypeNames =
{"id", "organ type", "sub type", "alive", "active", "age", "length", "1", "order", "parent type","creation time",
 "basal length", "apical length", "initial growth rate", "radius", "insertion angle", "root life time", "mean inter nodal distance", "standard deviation of inter nodal distance", "number of branches",
 "userdata 1", "userdata 2", "userdata 3"};

Plant::Plant()
{
	initOTP();
	setOrganTypeParameter(new SeedTypeParameter());
	seed = new Seed(this);
}

Plant::~Plant()
{
	for (auto& otp_:organParam) {
		for (auto& otp:otp_) {
			delete otp;
		}
	}
	delete seed;
	delete geometry;
}

/**
 *	Deletes the old parameter, sets the new one
 ************************************************not sure what to do******************************TODO*****
 */
void Plant::setOrganTypeParameter(OrganTypeParameter*  p)
{
//	 std::cout << "Plant::setOrganTypeParameter " << stem_p->organType << ", " << stem_p->subType <<"\n";

//	switch (OrganTypes()) {//check the type of the organ
//    std::cout<<"organtype "<<organType()<<" , subtype "<<stem_param<<std::endl;
	//used to debug and check organType and reference
//	case 1 :delete organParam.at(stem_p->organType).at(stem_p->subType);
//                            organParam.at(stem_p->organType).at(stem_p->subType) = stem_p;

//	case 2 :
	    delete organParam.at(p->organType).at(p->subType);
                            organParam.at(p->organType).at(p->subType) = p;
//	case 3 :
//delete organParam.at(stem_p->organType).at(stem_p->subType);
 //                           organParam.at(stem_p->organType).at(stem_p->subType) = stem_p;
//	case 4 :delete organParam.at(stem_p->organType).at(stem_p->subType);
//                            organParam.at(stem_p->organType).at(stem_p->subType) = stem_p;
//	case 5 :delete organParam.at(stem_p->organType).at(stem_p->subType);
//                            organParam.at(stem_p->organType).at(stem_p->subType) = stem_p;

//    }



}





OrganTypeParameter* Plant::getOrganTypeParameter(int otype, int subtype) const
{
//	std::cout << "Plant::getOrganTypeParameter " << otype << ", " << subtype <<"\n";
	return organParam.at(otype).at(subtype);
}

/**
 * Deletes the old geometry, sets the new one, passes it to the tropisms
 */
void Plant::setGeometry(SignedDistanceFunction* geom)
{
	delete geometry;
	geometry = geom;
	for (int i=0; i<maxtypes; i++) {
		RootTypeParameter* rtp = (RootTypeParameter*) getOrganTypeParameter(Organ::ot_root,i);
		if (rtp->subType>=0) { // defined
			delete rtp->tropism;
			rtp->createTropism(geom);
		}
	}
}

/**
 * Resets the root system: deletes all roots, does not change the parameters
 */
void Plant::reset()
{
	// delete seed; // TODO??????
	seed = new Seed(this);
	simtime=0;
	rid = -1;
	nid = -1;
}

/**
 * Puts default values into the root type parameters vector
 */
void Plant::initOTP()
{
	organParam = std::vector<std::vector<OrganTypeParameter*>>(maxorgans);
	for (auto& otp:organParam) {
		otp = std::vector<OrganTypeParameter*>(maxtypes);
		for (size_t i=0; i<otp.size(); i++) {
			otp.at(i) = new OrganTypeParameter();
		}
	}
}

/**
 * Reads the root parameter from a file. Opens plant parameters with the same filename if available,
 * othterwise assumes a tap root system at position (0,0,-3).
 *
 * @param name          filename without file extension
 * @param subdir        directory ("modelparameter/" by default)
 */
void Plant::openFile(std::string name, std::string subdir)
{
	std::ifstream fis;
	std::string rp_name = subdir;
	rp_name.append(name);
	rp_name.append(".rparam");
	fis.open(rp_name.c_str());
	int c = 0;
	if (fis.good()) { // did it work?
		c = readRootParameters(fis);
		fis.close();
	} else {
		std::string s = "RootSystem::openFile() could not open root parameter file ";
		throw std::invalid_argument(s.append(rp_name));
	}
	std::cout << "Read " << c << " root type parameters \n"; // debug

	// open seed parameter
	SeedTypeParameter* stp = (SeedTypeParameter*)getOrganTypeParameter(Organ::ot_seed,0);
	std::string pp_name = subdir;
	pp_name.append(name);
	pp_name.append(".pparam");
	fis.open(pp_name.c_str());
	if (fis.good()) { // did it work?
		stp->read(fis);
		fis.close();
	} else { // create a tap root system
		std::cout << "No seed system parameters found, using default tap root system \n";
		delete stp;
		setOrganTypeParameter(new SeedTypeParameter());
	}


// open stem parameter

	std::string stp_name = subdir;
	stp_name.append(name);
	stp_name.append(".stparam");
	fis.open(stp_name.c_str());
	int stem_c = 0;
	if (fis.good()) { // did it work?
		stem_c = readStemParameters(fis);
		fis.close();
	} else {
		std::string s = "stemSystem::openFile() could not open root parameter file ";
		throw std::invalid_argument(s.append(stp_name));
	}
	std::cout << "Read " << stem_c << " stem type parameters \n"; // debug

std::string lp_name = subdir;
	lp_name.append(name);
	lp_name.append(".leparam");
	fis.open(lp_name.c_str());
	int leaf_c = 0;
	if (fis.good()) { // did it work?
		leaf_c = readLeafParameters(fis);
		fis.close();
	} else {
		std::string s = "stemSystem::openFile() could not open leaf parameter file ";
		throw std::invalid_argument(s.append(lp_name));
	}
	std::cout << "Read " << leaf_c << " leaf type parameters \n"; // debug


}

/**
 * Reads parameter from input stream (there is a Matlab script exporting these, @see writeParams.m) TODO
 *
 * @param cin  in stream
 */
int Plant::readRootParameters(std::istream& cin)
{
	// initOTP();
	int c = 0;
	while (cin.good()) {
		RootTypeParameter* p  = new RootTypeParameter();
		p->read(cin);
		setOrganTypeParameter(p); // sets the param to the index (p.type-1) TODO
		c++;
	}
	return c;

}

int Plant::readStemParameters(std::istream& cin)
{
	// initOTP();
	int stem_c = 0;
	while (cin.good()) {
		StemTypeParameter* stem_p  = new StemTypeParameter();///added copypaste
		stem_p->read(cin);
		setOrganTypeParameter(stem_p);
		stem_c++;
	}
	return stem_c;

}


int Plant::readLeafParameters(std::istream& cin)
{
	// initOTP();
	int leaf_c = 0;
	while (cin.good()) {
		LeafTypeParameter* leaf_p  = new LeafTypeParameter();///added copypaste
		leaf_p->read(cin);
		setOrganTypeParameter(leaf_p);
		leaf_c++;
	}
	return leaf_c;

}
/**
 * Writes root parameters (for debugging) TODO
 *
 * @param os  out stream
 */
void Plant::writeParameters(std::ostream& os) const
{
	for (auto const& otp_:organParam) {
		int t = 0;
		for (auto const& otp : otp_) {
			if (otp->organType>=0 && (otp->subType>=0)) {
				assert(otp->subType==t); // check if index really equals subType-1
				os << otp->writeXML(0); // only write if defined
			}
			t++;
		}
	}
}

/**
 * Sets up the plant according to the given parameters
 */
void Plant::initialize(int basaltype, int shootbornetype)
{
	reset(); // deletes the old seed, makes a new one
	seed->initialize();

	/* the following code will be moved to the shoot */
	//  // Shoot borne roots
	//  if ((rs.nC>0) && (rs.delaySB<maxT)) { // if the type is not defined, copy basal root
	//      //		if (getRootTypeParameter(shootbornetype)->type<1) {
	//      //			std::cout << "Shootborne root type #" << shootbornetype << " was not defined, using tap root parameters instead\n";
	//      //			RootTypeParameter srtp = RootTypeParameter(*getRootTypeParameter(1));
	//      //			srtp.type = shootbornetype;
	//      //			setRootTypeParameter(srtp);
	//      //		}
	//      Vector3d sbpos = rs.seedPos;
	//      sbpos.z=sbpos.z/2.; // half way up the mesocotyl
	//      int maxSB = ceil((maxT-rs.firstSB)/rs.delayRC); // maximal number of root crowns
	//      double delay = rs.firstSB;
	//      for (int i=0; i<maxSB; i++) {
	//          //			for (int j=0; j<rs.nC; j++) {
	//          //				Organ* shootborne = new Organ(this, shootbornetype, iheading ,delay, nullptr, 0, 0);
	//          //				// TODO fix the initial radial heading
	//          //				shootborne->addNode(sbpos,delay);
	//          //				baseRoots.push_back(shootborne);
	//          //				delay += rs.delaySB;
	//          //			}
	//          sbpos.z+=rs.nz;  // move up, for next root crown
	//          delay = rs.firstSB + i*rs.delayRC; // reset age
	//      }
	//   }

}

/**
 * Simulates root system growth for time span dt
 *
 * @param dt    	time step [days]
 * @param silence 	indicates if status is written to the console (cout) (default = false)
 */
void Plant::simulate(double dt, bool silence)
{
	if (!silence) {
		std::cout << "Plant.simulate(dt) from "<< simtime << " to " << simtime+dt << " days \n";
	}
	getOrgans(Organ::ot_organ);
	old_non = getNumberOfNodes();
	old_nor = organs.size();
	seed->simulate(dt);
	simtime+=dt;
	organs.clear(); // empty buffer
	organs_type = -1;
}

/**
 * Simulates root system growth for the time span defined in the parameter set TODO
 */
void Plant::simulate()
{
	// this->simulate(seedParam->simtime); TODO
}

/**
 * Sets the seed of the root systems random number generator,
 * and all subclasses using random number generators:
 * @see TropismFunction, @see RootParameter
 *
 * @param seed      random number generator seed
 */
void Plant::setSeed(unsigned int seed) const {
	//  TODO
}

/**
 *
 */
std::vector<Organ*> Plant::getOrgans(unsigned int otype) const
{
	if (organs_type!=otype) { // create buffer
		organs = seed->getOrgans(otype);
		organs_type = otype;
		return organs;
	} else { // return buffer
		return organs;
	}
}

/**
 * Returns the node indices of the root tips
 */
std::vector<int> Plant::getRootTips() const
{
	this->getOrgans(Organ::ot_root); // update roots (if necessary)
	std::vector<int> tips;
	for (auto& r : organs) {
		Root* r2 = (Root*)r;
		tips.push_back(r2->getNodeID(r2->getNumberOfNodes()-1));
	}
	return tips;
}

/**
 * Returns the positions of the root bases
 */
std::vector<int> Plant::getRootBases() const
{
	this->getOrgans(Organ::ot_root); // update roots (if necessary)  std::vector<int> bases;
	std::vector<int> bases;
	for (auto& r : organs) {
		Root* r2 = (Root*)r;
		bases.push_back(r2->getNodeID(0));
	}
	return bases;
}

/**
 * Copies the nodes of the root systems into a sequential vector,
 * nodes are unique (default). See also RootSystem::getSegments
 */
std::vector<Vector3d> Plant::getNodes() const
{
	this->getOrgans(Organ::ot_organ); // update roots (if necessary)
	int non = getNumberOfNodes();
	std::vector<Vector3d> nv = std::vector<Vector3d>(non); // reserve big enough vector
	for (auto const& r: organs) {
		for (size_t i=0; i<r->getNumberOfNodes(); i++) { // loop over all nodes of all roots
			nv.at(r->getNodeID(i)) = r->getNode(i); // pray that ids are correct
		}
	}
	return nv;
}

/**
 * Returns the root system as polylines, i.e. each root is represented by its nodes
 */
std::vector<std::vector<Vector3d>> Plant::getPolylines(unsigned int otype) const
{
	this->getOrgans(otype); // update roots (if necessary)
	std::vector<std::vector<Vector3d>> nodes = std::vector<std::vector<Vector3d>>(organs.size()); // reserve big enough vector
	for (size_t j=0; j<organs.size(); j++) {
		std::vector<Vector3d>  rn = std::vector<Vector3d>(organs[j]->getNumberOfNodes());
		for (size_t i=0; i<organs[j]->getNumberOfNodes(); i++) { // loop over all nodes of all roots
			rn.at(i) = organs[j]->getNode(i);
		}
		nodes[j] = rn;
	}
	return nodes;
}

/**
 * Return the segments of the root system at the current simulation time
 */
std::vector<Vector2i> Plant::getSegments(unsigned int otype) const
{
	this->getOrgans(otype); // update roots (if necessary)
	int nos=getNumberOfSegments();
	std::vector<Vector2i> s(nos);
	int c=0;
	for (auto const& r:organs) {
		for (size_t i=0; i<r->getNumberOfNodes()-1; i++) {
			Vector2i v(r->getNodeID(i),r->getNodeID(i+1));
			s.at(c) = v;
			c++;
		}
	}
	return s;
}

/**
 * Returns pointers to the organs corresponding to each segment
 */
std::vector<Organ*> Plant::getSegmentsOrigin(unsigned int otype) const
{
	this->getOrgans(otype); // update (if necessary)
	int nos=getNumberOfSegments();
	std::vector<Organ*> s(nos);
	int c=0;
	for (auto const& o:organs) {
		for (size_t i=0; i<o->getNumberOfNodes()-1; i++) {
			s.at(c) = o;
			c++;
		}
	}
	return s;
}

/**
 * Copies the node emergence times of the root systems into a sequential vector,
 * see RootSystem::getNodes()
 */
std::vector<double> Plant::getNETimes() const
{
	this->getOrgans(Organ::ot_organ); // update roots (if necessary)
	int nos=getNumberOfSegments();
	std::vector<double> netv = std::vector<double>(nos); // reserve big enough vector
	int c=0;
	for (auto const& r: organs) {
		for (size_t i=1; i<r->getNumberOfNodes(); i++) { // loop over all nodes of all roots
			netv.at(c) = r->getNodeCT(i); // pray that ids are correct
			c++;
		}
	}
	return netv;
}

/**
 *  Returns the node emergence times to the corresponding polylines, see also RootSystem::getPolylines
 */
std::vector<std::vector<double>> Plant::getPolylinesNET(unsigned int otype) const
{
	this->getOrgans(otype); // update roots (if necessary)
	std::vector<std::vector<double>> times = std::vector<std::vector<double>>(organs.size()); // reserve big enough vector
	for (size_t j=0; j<organs.size(); j++) {
		std::vector<double>  rt = std::vector<double>(organs[j]->getNumberOfNodes());
		for (size_t i=0; i<organs[j]->getNumberOfNodes(); i++) {
			rt[i] = organs[j]->getNodeCT(i);
		}
		times[j] = rt;
	}
	return times;
}


/**
 * Copies a scalar that is constant per organ to a sequential vector (one scalar per organ).
 *
 * @param stype     a scalar type (@see RootSystem::ScalarTypes). st_time is the emergence time of the root
 */
std::vector<double> Plant::getScalar(unsigned int otype, int stype) const
{
	this->getOrgans(otype); // update roots (if necessary)
	std::vector<double> scalars(organs.size());
	for (size_t i=0; i<organs.size(); i++) {
		const auto& o = organs[i];
		double value = o->getScalar(stype);
		//      switch(stype) {
		//        case st_type:  // type
		//          value = organs[i]->param.type;
		//          break;
		//        case st_radius: // root radius
		//          value = organs[i]->param.a;
		//          break;
		//        case st_order: { // root order (calculate)
		//          value = 0;
		//          Organ* r_ = organs[i];
		//          while (r_->parent!=nullptr) {
		//              value++;
		//              r_=r_->parent;
		//          }
		//          break;
		//        }
		//        case st_time: // emergence time of the root
		//          value = organs[i]->getNodeETime(0);
		//          break;
		//        case st_length:
		//          value = organs[i]->length;
		//          break;
		//        case st_surface:
		//          value =  organs[i]->length*2.*M_PI*organs[i]->param.a;
		//          break;
		//        case st_one:
		//          value =  1;
		//          break;
		//        case st_parenttype: {
		//          Organ* r_ = organs[i];
		//          if (r_->parent!=nullptr) {
		//              value = r_->parent->param.type;
		//          } else {
		//              value = 0;
		//          }
		//          break;
		//        }
		//        case st_lb:
		//          value = organs[i]->param.lb;
		//          break;
		//        case st_la:
		//          value = organs[i]->param.la;
		//          break;
		//        case st_nob:
		//          value = organs[i]->param.nob;
		//          break;
		//        case st_r:
		//          value = organs[i]->param.r;
		//          break;
		//        case st_theta:
		//          value = organs[i]->param.theta;
		//          break;
		//        case st_rlt:
		//          value = organs[i]->param.rlt;
		//          break;

		scalars[i]=value;
	}
	return scalars;
}

/**
 * todo
 */
std::string Plant::toString() const
{
	return "todo";
}


/**
 * Exports the simulation results with the type from the extension in name
 * (that must be lower case)
 *
 * @param name      file name e.g. output.vtp
 */
void Plant::write(std::string name, int otype) const
{
	std::ofstream fos;
	fos.open(name.c_str());
	std::string ext = name.substr(name.size()-3,name.size()); // pick the right writer
	if (ext.compare("sml")==0) {
		std::cout << "writing RSML... "<< name.c_str() <<"\n";
		writeRSML(fos);
	} else if (ext.compare("vtp")==0) {
		std::cout << "writing VTP... "<< name.c_str() <<"\n";
		writeVTP(otype, fos);
	} else if (ext.compare(".py")==0)  {
		std::cout << "writing Geometry ... "<< name.c_str() <<"\n";
		writeGeometry(fos);
	} else {
		throw std::invalid_argument("RootSystem::write(): Unkwown file type");
	}
	fos.close();
}

/**
 * Creates an RSML file
 *
 * @param os      typically a file out stream
 */
void Plant::writeRSML(std::ostream & os) const
{
	os << "<?xml version=\"1.0\" encoding=\"ISO-8859-1\"?>\n"; // i am not using utf-8, but not sure if ISO-8859-1 is correct
	os << "<rsml>\n";
	writeRSMLMeta(os);
	os<< "<scene>\n";
	writeRSMLPlant(os);
	os << "</scene>\n";
	os << "</rsml>\n";
}

/**
 * Writes RML meta data tag
 *
 * @param os      typically a file out stream
 */
void Plant::writeRSMLMeta(std::ostream & os) const
{
	os << "<metadata>\n";
	os << "\t<version>" << 1 << "</version>\n";
	os << "\t<unit>" << "cm" << "</unit>\n";
	os << "\t<resolution>" << 1 << "</resolution>\n";
	// fetch time
	//    os << "<last-modified>";
	//    auto t = std::time(nullptr);
	//    auto tm = *std::localtime(&t);
	//    os << std::put_time(&tm, "%d-%m-%Y"); // %H-%M-%S" would do the job for gcc 5.0
	//    os << "</last-modified>\n";
	os << "\t<software>CRootBox</software>\n";
	os << "</metadata>\n";
}

/**
 * Writes RSML plant tag
 *
 * @param os      typically a file out stream
 */
void Plant::writeRSMLPlant(std::ostream & os) const
{
	os << "<plant>\n";
	seed->writeRSML(os,"");
	os << "</plant>\n";
}

/**
 * Writes current simulation results as VTP (VTK polydata file),
 * where each root is represented by a polyline.
 *
 * Use SegmentAnalyser::writeVTP() for a representation based on segments,
 * e.g. for creating a movie (and run the animate.py script), or mapping values to segments
 *
 * @param os      typically a file out stream
 */
void Plant::writeVTP(int otype, std::ostream & os) const
{
	this->getOrgans(otype); // update roots (if necessary)
	const auto& nodes = getPolylines(otype);
	const auto& times = getPolylinesNET(otype);

	os << "<?xml version=\"1.0\"?>";
	os << "<VTKFile type=\"PolyData\" version=\"0.1\" byte_order=\"LittleEndian\">\n";
	os << "<PolyData>\n";
	int non = 0; // number of nodes
	for (auto const& r : organs) {
		non += r->getNumberOfNodes();
	}
	int nol=organs.size(); // number of lines
	os << "<Piece NumberOfLines=\""<< nol << "\" NumberOfPoints=\""<<non<<"\">\n";

	// POINTDATA
	os << "<PointData Scalars=\" PointData\">\n" << "<DataArray type=\"Float32\" Name=\"time\" NumberOfComponents=\"1\" format=\"ascii\" >\n";
	for (const auto& r: times) {
		for (const auto& t : r) {
			os << t << " ";
		}
	}
	os << "\n</DataArray>\n" << "\n</PointData>\n";

	// CELLDATA (live on the polylines)
	os << "<CellData Scalars=\" CellData\">\n";
	const size_t N = 3; // SCALARS
	int types[N] = { st_subtype, st_order, st_radius };
	std::vector<std::string> sTypeNames = { "type", "order", "radius"};
	for (size_t i=0; i<N; i++) {
		os << "<DataArray type=\"Float32\" Name=\"" << sTypeNames[i] <<"\" NumberOfComponents=\"1\" format=\"ascii\" >\n";
		auto scalars = getScalar(otype, types[i]);
		for (auto s : scalars) {
			os << s<< " ";
		}
		os << "\n</DataArray>\n";
	}
	os << "\n</CellData>\n";

	// POINTS (=nodes)
	os << "<Points>\n"<<"<DataArray type=\"Float32\" Name=\"Coordinates\" NumberOfComponents=\"3\" format=\"ascii\" >\n";
	for (auto const& r:nodes) {
		for (auto const& n : r) {
			os << n.x << " "<< n.y <<" "<< n.z<< " ";
		}
	}
	os << "\n</DataArray>\n"<< "</Points>\n";

	// LINES (polylines)
	os << "<Lines>\n"<<"<DataArray type=\"Int32\" Name=\"connectivity\" NumberOfComponents=\"1\" format=\"ascii\" >\n";
	int c=0;
	for (auto const& r:organs) {
		for (size_t i=0; i<r->getNumberOfNodes(); i++) {
			os << c << " ";
			c++;
		}
	}
	os << "\n</DataArray>\n"<<"<DataArray type=\"Int32\" Name=\"offsets\" NumberOfComponents=\"1\" format=\"ascii\" >\n";
	c = 0;
	for (auto const& r:organs) {
		c += r->getNumberOfNodes();
		os << c << " ";
	}
	os << "\n</DataArray>\n";
	os << "\n</Lines>\n";

	os << "</Piece>\n";
	os << "</PolyData>\n" << "</VTKFile>\n";
}

/**
 * Writes the current confining geometry (e.g. a plant container) as paraview python script
 * Just adds the initial lines, before calling the method of the sdf.
 *
 * @param os      typically a file out stream
 */
void Plant::writeGeometry(std::ostream & os) const
{
	os << "from paraview.simple import *\n";
	os << "paraview.simple._DisableFirstRenderCameraReset()\n";
	os << "renderView1 = GetActiveViewOrCreate('RenderView')\n\n";
	geometry->writePVPScript(os);
}

