#include <RcppArmadillo.h>
#include <cmath>
#include <typeinfo>
#include <omp.h>


// [[Rcpp::export]]
arma::mat convertToPrecision(arma::vec Delta, arma::uword q1, arma::uword q2);
arma::mat getPenalty(arma::uword n);
double sampleTrunc(double shape, double scale, double cutoff);
void updateBeta(arma::cube &eta, arma::mat &H, arma::mat &E, arma::mat &X, arma::mat &beta){
  int q = eta.n_rows;
  int qstar = eta.n_cols;
  // X is design matrix. Each row for each patient. Number of columns for number of covariates.
  int d = X.n_cols;
  //arma::mat beta = arma::zeros(d, q*qstar);

  // Prior mean
  arma::vec mu = arma::zeros(d);

  // Thinking of M&m formula
  arma::mat Minv = arma::zeros(d, d);
  arma::mat M = arma::zeros(d, d);
  arma::vec m = arma::zeros(d);
  int idx = 0;
  /*
  for(arma::uword(i) = 0; i < arma::uword(q); i++){
    for(arma::uword(j) = 0; j < arma::uword(qstar); j++){
      idx = arma::sub2ind(arma::size(eta.slice(0)), i, j);
      //Minv = (arma::trans(X) * X) + precision; // H IS DIAGONAL H(i, j) DOES NOT MAKE SENSE
      Minv = (arma::trans(X) * X) + arma::diagmat(E.row(idx));
      M = arma::inv_sympd(Minv);
      //m = arma::trans(X) * arma::vec(eta.tube(i, j)) + precision * mu;
      m = arma::trans(X) * arma::vec(eta.tube(i, j));
      beta.col(idx) = M * m + 1.0/sqrt(H(idx,idx)) * arma::chol(M, "lower") * arma::randn<arma::vec>(d);
    }
  }*/
  for(arma::uword(i) = 0; i < arma::uword(q); i++){
    for(arma::uword(j) = 0; j < arma::uword(qstar); j++){
      idx = arma::sub2ind(arma::size(eta.slice(0)), i, j);
      //Minv = (arma::trans(X) * X) + precision; // H IS DIAGONAL H(i, j) DOES NOT MAKE SENSE
      Minv = H(idx,idx)*(arma::trans(X) * X) + arma::diagmat(E.row(idx));
      M = arma::inv_sympd(Minv);
      //m = arma::trans(X) * arma::vec(eta.tube(i, j)) + precision * mu;
      m = H(idx,idx)*arma::trans(X) * arma::vec(eta.tube(i, j));
      beta.col(idx) = M * m + arma::chol(M, "lower") * arma::randn<arma::vec>(d);
    }
  }
  //return beta;
}

// [[Rcpp::depends(RcppArmadillo)]]
// [[Rcpp::export]]
arma::mat updateBeta2(arma::mat E, arma::mat Eta, arma::mat X){
  arma::uword r = X.n_cols;
  arma::uword k = Eta.n_cols;
  arma::mat Beta(r, k);
  arma::mat cov;
  arma::mat mu;
  //Rcpp::Rcout << E;
  for(arma::uword i = 0; i < k; i++){
    cov = arma::inv_sympd(arma::trans(X) * X + arma::diagmat(E.row(i)));
    mu = cov * arma::trans(X) * Eta.col(i);
    Beta.col(i) = mu + arma::chol(cov, "lower") * arma::randn<arma::vec>(r);
  }
  return(Beta);
}

// [[Rcpp::export]]
void updateBetaProd(arma::cube &eta, arma::vec Delta, arma::mat &E, arma::mat &X, arma::mat &beta){
  int q = eta.n_rows;
  int qstar = eta.n_cols;
  arma::mat H = convertToPrecision(Delta, q, qstar);
  // X is design matrix. Each row for each patient. Number of columns for number of covariates.
  int d = X.n_cols;
  //arma::mat beta = arma::zeros(d, q*qstar);

  // Prior mean
  arma::vec mu = arma::zeros(d);

  // Thinking of M&m formula
  arma::mat Minv = arma::zeros(d, d);
  arma::mat M = arma::zeros(d, d);
  arma::vec m = arma::zeros(d);
  int idx = 0;
  /*
  for(arma::uword(i) = 0; i < arma::uword(q); i++){
  for(arma::uword(j) = 0; j < arma::uword(qstar); j++){
  idx = arma::sub2ind(arma::size(eta.slice(0)), i, j);
  //Minv = (arma::trans(X) * X) + precision; // H IS DIAGONAL H(i, j) DOES NOT MAKE SENSE
  Minv = (arma::trans(X) * X) + arma::diagmat(E.row(idx));
  M = arma::inv_sympd(Minv);
  //m = arma::trans(X) * arma::vec(eta.tube(i, j)) + precision * mu;
  m = arma::trans(X) * arma::vec(eta.tube(i, j));
  beta.col(idx) = M * m + 1.0/sqrt(H(idx,idx)) * arma::chol(M, "lower") * arma::randn<arma::vec>(d);
  }
  }*/
  for(arma::uword(i) = 0; i < arma::uword(q); i++){
    for(arma::uword(j) = 0; j < arma::uword(qstar); j++){
      idx = arma::sub2ind(arma::size(eta.slice(0)), i, j);
      //Minv = (arma::trans(X) * X) + precision; // H IS DIAGONAL H(i, j) DOES NOT MAKE SENSE
      Minv = H(idx,idx)*(arma::trans(X) * X) + arma::diagmat(E.row(idx));
      M = arma::inv_sympd(Minv);
      //m = arma::trans(X) * arma::vec(eta.tube(i, j)) + precision * mu;
      m = H(idx,idx)*arma::trans(X) * arma::vec(eta.tube(i, j));
      beta.col(idx) = M * m + arma::chol(M, "lower") * arma::randn<arma::vec>(d);
    }
  }
  //return beta;
}

arma::vec updateDelta(arma::mat &Lambda, arma::mat &Phi, arma::vec Delta, double a1, double a2){
  arma::vec output;
  output.copy_size(Delta);
  double p = Lambda.n_rows;
  double k = Lambda.n_cols;
  // I think this code is only ran once

   arma::uword num = Delta.n_elem;
   arma::umat A(num-1,num-2);
   for(arma::uword col = 0; col < num-2; ++col){
   for(arma::uword row = 0; row < num - 1; ++row){
   if(row > col){
   A(row, col) = col + 1;
   }
   else {
   A(row, col) = col + 2;
   }
   }
   }
   A.insert_cols(0, arma::zeros<arma::uvec>(num-1));

  arma::rowvec colsums = sum(Phi % arma::square(Lambda), 0);
  arma::vec tauH = arma::cumprod(Delta);

  // temporary for delta
  arma::vec tempD = tauH;
  tempD(0) = 1.0;
  Delta(0) = R::rgamma(a1 + p*k / 2.0, 1.0 / (1.0 + 1.0/2.0 * arma::dot(tempD, colsums)));
  // temporary for other delta, reusing tempD
  for(arma::uword i = 1; i < Delta.n_elem; ++i){
    tempD = cumprod(Delta(A.row(i - 1)));
    double tempSum = arma::dot(tempD.tail(Delta.n_elem - i), colsums.tail(Delta.n_elem - i));
    Delta(i) = R::rgamma(a2 + p*(k-i)/2.0, 1.0 / (1.0 + 1.0/2.0 * tempSum));
  }
  return Delta;
}

arma::vec updateDeltastar(arma::mat &Gamma, arma::mat &Phistar, arma::vec Deltastar, double a1star, double a2star){
  arma::vec output;
  output.copy_size(Deltastar);
  double pstar = Gamma.n_rows;
  double kstar = Gamma.n_cols;
  // I think this code is only ran once

  arma::uword num = Deltastar.n_elem;
  arma::umat Astar(num-1,num-2);
  for(arma::uword col = 0; col < num-2; ++col){
    for(arma::uword row = 0; row < num - 1; ++row){
      if(row > col){
        Astar(row, col) = col + 1;
      }
      else {
        Astar(row, col) = col + 2;
      }
    }
  }
  Astar.insert_cols(0, arma::zeros<arma::uvec>(num-1));

  arma::rowvec colsums = sum(Phistar % arma::square(Gamma), 0);
  arma::vec tauH = arma::cumprod(Deltastar);

  // temporary for delta
  arma::vec tempD = tauH;
  tempD(0) = 1.0;
  Deltastar(0) = R::rgamma(a1star + pstar*kstar / 2.0, 1.0 / (1.0 + 1.0 / 2.0 * arma::dot(tempD, colsums)));
  // temporary for other delta, reusing tempD
  for(arma::uword i = 1; i < Deltastar.n_elem; ++i){
    tempD = arma::cumprod(Deltastar(Astar.row(i - 1)));
    double tempSum = arma::dot(tempD.tail(Deltastar.n_elem - i), colsums.tail(Deltastar.n_elem - i));
    Deltastar(i) = R::rgamma(a2star + pstar*(kstar-double(i))/2.0, 1.0 / (1.0 + 1.0/2.0 * tempSum));
  }
  return Deltastar;
}

// [[Rcpp::depends(RcppArmadillo)]]
// [[Rcpp::export]]
arma::vec updateDelta3(arma::mat &Lambda, arma::mat &Phi, arma::vec Delta, double a1, double a2){
  arma::vec output;
  output.copy_size(Delta);
  double p = Lambda.n_rows;
  double k = Lambda.n_cols;
  // I think this code is only ran once

  arma::uword num = Delta.n_elem;
  arma::umat A(num-1,num-2);
  for(arma::uword col = 0; col < num-2; ++col){
  for(arma::uword row = 0; row < num - 1; ++row){
  if(row > col){
  A(row, col) = col + 1;
  }
  else {
  A(row, col) = col + 2;
  }
  }
  }
  A.insert_cols(0, arma::zeros<arma::uvec>(num-1));

  arma::rowvec colsums = sum(Phi % arma::square(Lambda), 0);
  arma::vec tauH = arma::cumprod(Delta);

  // temporary for delta
  arma::vec tempD = tauH;
  tempD(0) = 1.0;
  Delta(0) = R::rgamma(a1 + p*k / 2.0, 1.0 / (1.0 + 1.0/2.0 * arma::dot(tempD, colsums)));
  // temporary for other delta, reusing tempD
  for(arma::uword i = 1; i < Delta.n_elem; ++i){
    tempD = cumprod(Delta(A.row(i - 1)));
    double tempSum = arma::dot(tempD.tail(Delta.n_elem - i), colsums.tail(Delta.n_elem - i));
    Delta(i) = R::rgamma(a2 + p*(k-i)/2.0, 1.0 / (1.0 + 1.0/2.0 * tempSum));
  }
  return Delta;
}
// [[Rcpp::export]]
arma::vec updateDeltaProdTemp(arma::mat Lambda, arma::vec Delta, double a1, double a2){
  arma::mat Penalty = getPenalty(Lambda.n_rows);
  arma::vec output;
  output.copy_size(Delta);
  double p = Lambda.n_rows;
  double k = Lambda.n_cols;
  // I think this code is only ran once

  arma::uword num = Delta.n_elem;
  arma::umat A(num-1,num-2);
  for(arma::uword col = 0; col < num-2; ++col){
    for(arma::uword row = 0; row < num - 1; ++row){
      if(row > col){
        A(row, col) = col + 1;
      }
      else {
        A(row, col) = col + 2;
      }
    }
  }
  A.insert_cols(0, arma::zeros<arma::uvec>(num-1));

  arma::rowvec colsums(Lambda.n_cols);
  for(arma::uword j = 0; j < Lambda.n_cols; j++){
    colsums(j) = arma::as_scalar(arma::trans(Lambda.col(j)) * Penalty * Lambda.col(j));
  }
  arma::vec tauH = arma::cumprod(Delta);

  // temporary for delta
  arma::vec tempD = tauH;
  tempD(0) = 1.0;
  Delta(0) = R::rgamma(a1 + p*k / 2.0, 1.0 / (1.0 + 1.0/2.0 * arma::dot(tempD, colsums)));
  // temporary for other delta, reusing tempD
  for(arma::uword i = 1; i < Delta.n_elem; ++i){
    tempD = cumprod(Delta(A.row(i - 1)));
    double tempSum = arma::dot(tempD.tail(Delta.n_elem - i), colsums.tail(Delta.n_elem - i));
    Delta(i) = R::rgamma(a2 + p*(k-i)/2.0, 1.0 / (1.0 + 1.0/2.0 * tempSum));
  }
  return Delta;
}
// [[Rcpp::plugins(openmp)]]
// [[Rcpp::export]]
arma::mat updateE(arma::mat beta){
  double a = 1.0 / 2.0;
  double b = 1.0 / 2.0;
  int q = beta.n_cols;
  int d = beta.n_rows;
  arma::mat E(q, d);
  for(int i = 0; i < q; i++){
    for(int j = 0; j < d; j++){
      E(i, j) = R::rgamma(a + 1.0 / 2.0, 1.0 / (b + ::pow(beta(j,i), 2.0)));
    }
  }

  return E;

}

// [[Rcpp::plugins(openmp)]]
// [[Rcpp::export]]
void updateEta(arma::mat &Lambda, arma::mat &Gamma, arma::vec sigma1,
                     arma::vec sigma2, arma::mat H, arma::mat &splineS,
                     arma::mat &splineT, arma::mat &y, double varphi,
                     arma::mat &beta, arma::mat &X, arma::cube &eta){
  int p = arma::size(sigma1, 0);
  int pstar = arma::size(sigma2, 0);
  int q = arma::size(Lambda, 1);
  int qstar = arma::size(Gamma, 1);
  arma::vec sigvec(p * pstar);
  arma::uword n = arma::size(y, 1);
  //arma::cube eta(q, qstar, n);
  arma::mat precision;
  //arma::vec mu;
  arma::mat cov;
  arma::mat sigmat = arma::diagmat(arma::kron(sigma2, sigma1));
  /*for(arma::uword i = 0; i < arma::uword(pstar); i++){
   for(arma::uword j = 0; j < arma::uword(p); j++){
   sigvec(p*i + j) = sigma2(i) * sigma1(j);
   }
  }
   arma::mat sigmat = arma::diagmat(sigvec);
   */

  //arma::mat spline = arma::kron(splineS, splineT);
  arma::mat BkronGL = arma::kron(splineS * Gamma, splineT * Lambda);

  //arma::mat temp = arma::trans(arma::inv(arma::trimatu(arma::chol(splineM(0) * arma::diagmat(sigmat) *
  //  arma::trans(splineM(0)) + 1.0 / varphi * arma::eye(splineM(0).n_rows, splineM(0).n_rows))))) * splineM(0) * kronGL;
  arma::mat Identity = arma::eye(splineS.n_rows * splineT.n_rows, splineS.n_rows * splineT.n_rows);
  arma::mat Woodbury = varphi * Identity - ::pow(varphi, 2.0) * arma::kron(splineS, splineT) *
    arma::inv_sympd(sigmat + varphi * arma::kron(arma::trans(splineS) * splineS,
                                              arma::trans(splineT) * splineT)) * arma::trans(arma::kron(splineS, splineT));
  //arma::mat P;
  //arma::mat Q;
  //arma::vec Lambda1;
  //arma::vec Lambda2;
  //arma::eig_sym(Lambda1, P, splineS * arma::inv(arma::diagmat(sigma2)) * arma::trans(splineS));
  //arma::eig_sym(Lambda2, Q, splineT * arma::inv(arma::diagmat(sigma1)) * arma::trans(splineT));
  //arma::mat precision = arma::kron(P * splineS * Gamma, Q * splineT * Lambda).t() * arma::inv(arma::diagmat((varphi * Identity + arma::diagmat(arma::kron(Lambda1, Lambda2))))) * arma::kron(P * splineS * Gamma, Q * splineT * Lambda) + H;
  //arma::mat Woodbury = arma::eye(splineS.n_rows * splineT.n_rows, splineS.n_rows * splineT.n_rows);
  //precision = arma::kron(arma::trans(Gamma) * arma::trans(splineS), arma::trans(Lambda) * arma::trans(splineT)) * arma::solve(
    //arma::kron(splineS * arma::diagmat(sigma2) * arma::trans(splineS), splineT * arma::diagmat(sigma1) * arma::trans(splineT)) + varphi * Identity,
    //arma::kron(splineS * Gamma, splineT * Lambda));
  //)
  //precision = arma::trans(temp) * temp + H;
  //arma::mat cholwood = arma::chol(sigmat + varphi * arma::kron(arma::trans(splineS) * splineS, arma::trans(splineT) * splineT), "lower");
  precision = arma::trans(BkronGL) *  Woodbury * BkronGL + H;



  cov = arma::inv_sympd(precision);
  arma::mat cholcov = arma::chol(cov, "lower");

  //omp_set_num_threads(12);
  //#pragma omp parallel
  //{
    arma::vec mu;
    //#pragma omp for schedule(static)
    for(arma::uword i = 0; i < n; i++){
      mu = cov * (H * arma::trans(beta)*arma::trans(X.row(i)) + arma::trans(BkronGL) * Woodbury * y.col(i));
      /*mu = cov * (H * arma::trans(beta) * arma::trans(X.row(i)) + arma::kron(P * splineS * Gamma, Q *
        splineT * Lambda).t() * arma::inv(arma::diagmat((varphi * Identity +
        arma::diagmat(arma::kron(Lambda1, Lambda2))))) * arma::kron(P, Q) * y(i));*/
      eta.slice(i) = arma::reshape(mu + cholcov * arma::randn<arma::vec>(arma::uword(q * qstar)), q, qstar);
    }
  //}
  //return eta;
}

// [[Rcpp::depends(RcppArmadillo)]]
// [[Rcpp::export]]
arma::mat updateEta2(arma::mat Lambda, arma::mat Basis, arma::vec Sigma, arma::mat Data, arma::mat Beta, arma::mat X, double Varphi){
  arma::mat Eta(Data.n_cols, Lambda.n_cols);
  //arma::mat Woodbury = Varphi * arma::eye(Data.n_rows, Data.n_rows) -
  //Varphi * Varphi * Basis * arma::inv_sympd(arma::diagmat(Sigma) + arma::trans(Basis) * Varphi * Basis) * arma::trans(Basis);
  //arma::mat cov = arma::inv_sympd(arma::trans(Lambda) *
  //arma::trans(Basis) * Woodbury * Basis * Lambda + arma::eye(Lambda.n_cols, Lambda.n_cols));
  //Rcpp::Rcout << arma::trans(Lambda) *
  //arma::trans(Basis) * Woodbury * Basis * Lambda << std::endl;

  arma::mat Precision = arma::trans(Lambda) * arma::trans(Basis) * arma::inv_sympd(1.0 / Varphi * arma::eye(Data.n_rows, Data.n_rows) +
    Basis * arma::inv(arma::diagmat(Sigma)) * arma::trans(Basis)) * Basis * Lambda + arma::eye(Lambda.n_cols, Lambda.n_cols);

  arma::mat cov = arma::inv_sympd(Precision);
  arma::vec mu;
  for(arma::uword i = 0; i < Data.n_cols; i++){
    //mu = cov * (arma::trans(Beta) * X.col(i) + arma::trans(Lambda) * arma::trans(Basis) * Woodbury * Data.col(i));
    //arma::mat B = arma::trans(Basis) * arma::inv_sympd(1.0 / Varphi + Basis * arma::inv(arma::diagmat(Sigma)) * arma::trans(Basis)) * Data.col(i);
    //Rcpp::Rcout << arma::eye(1,10) * arma::trans(Basis) * arma::inv_sympd(1.0 / Varphi * arma::eye(Data.n_rows, Data.n_rows) + Basis * arma::inv(arma::diagmat(Sigma)) * arma::trans(Basis));// * Data.col(i);
    //Rcpp::Rcout << arma::eye(1,10) * arma::trans(Basis) * arma::inv_sympd(1.0 / Varphi + Basis * arma::inv(arma::diagmat(Sigma)) * arma::trans(Basis)) * Data.col(i);
    //Rcpp::Rcout << "Here1";
    //arma::trans(Beta) * X.col(i) + arma::trans(Lambda) * arma::trans(Basis) * arma::trans(Basis) * Data.col(i);
    mu = cov * (arma::trans(Beta) * arma::trans(X.row(i)) + arma::trans(Lambda) * arma::trans(Basis) * arma::inv_sympd(1.0 / Varphi * arma::eye(Data.n_rows, Data.n_rows) + Basis * arma::inv(arma::diagmat(Sigma)) * arma::trans(Basis)) * Data.col(i));

    Eta.row(i) = arma::trans(mu + arma::chol(cov, "lower") * arma::randn<arma::vec>(Lambda.n_cols));

  }

  return(Eta);
}
// [[Rcpp::export]]
void updateEta3(arma::mat Gamma, arma::mat Lambda, arma::vec sigma2, arma::vec sigma1, arma::cube Theta, arma::mat H, arma::mat X, arma::mat Beta, arma::cube &eta){
  int q = arma::size(Lambda, 1);
  int qstar = arma::size(Gamma, 1);
  arma::mat kronGL = arma::kron(Gamma, Lambda);
  arma::mat sigmat = arma::diagmat(arma::kron(sigma2, sigma1));
  arma::mat precision = arma::trans(kronGL) * sigmat * kronGL + H;
  arma::mat cov = arma::inv_sympd(precision);
  arma::vec mu;
  arma::mat cholcov = arma::chol(cov, "lower");
  for(arma::uword i = 0; i < arma::size(Theta, 2); i++){

    mu = cov * (H * arma::trans(Beta) * arma::trans(X.row(i)) + arma::trans(kronGL) * sigmat * arma::vectorise(Theta.slice(i)));
    eta.slice(i) = arma::reshape(mu + cholcov * arma::randn<arma::vec>(arma::uword(q * qstar)), q, qstar);


  }

}
// [[Rcpp::export]]

void updateEtaProd(arma::mat &Lambda, arma::mat &Gamma, arma::vec sigma1,
               arma::vec sigma2, arma::vec Delta, arma::mat &splineS,
               arma::mat &splineT, arma::mat &y, double varphi,
               arma::mat &beta, arma::mat &X, arma::cube &eta){
  int p = arma::size(sigma1, 0);
  int pstar = arma::size(sigma2, 0);
  int q = arma::size(Lambda, 1);
  int qstar = arma::size(Gamma, 1);
  arma::mat H = convertToPrecision(Delta, q, qstar);

  arma::vec sigvec(p * pstar);
  arma::uword n = arma::size(y, 1);
  //arma::cube eta(q, qstar, n);
  arma::mat precision;
  //arma::vec mu;
  arma::mat cov;
  arma::mat sigmat = arma::diagmat(arma::kron(sigma2, sigma1));
  /*for(arma::uword i = 0; i < arma::uword(pstar); i++){
  for(arma::uword j = 0; j < arma::uword(p); j++){
  sigvec(p*i + j) = sigma2(i) * sigma1(j);
  }
}
  arma::mat sigmat = arma::diagmat(sigvec);
  */

  //arma::mat spline = arma::kron(splineS, splineT);
  arma::mat BkronGL = arma::kron(splineS * Gamma, splineT * Lambda);

  //arma::mat temp = arma::trans(arma::inv(arma::trimatu(arma::chol(splineM(0) * arma::diagmat(sigmat) *
  //  arma::trans(splineM(0)) + 1.0 / varphi * arma::eye(splineM(0).n_rows, splineM(0).n_rows))))) * splineM(0) * kronGL;
  arma::mat Identity = arma::eye(splineS.n_rows * splineT.n_rows, splineS.n_rows * splineT.n_rows);
  arma::mat Woodbury = varphi * Identity - ::pow(varphi, 2.0) * arma::kron(splineS, splineT) *
    arma::inv_sympd(sigmat + varphi * arma::kron(arma::trans(splineS) * splineS,
                                                 arma::trans(splineT) * splineT)) * arma::trans(arma::kron(splineS, splineT));
  //arma::mat P;
  //arma::mat Q;
  //arma::vec Lambda1;
  //arma::vec Lambda2;
  //arma::eig_sym(Lambda1, P, splineS * arma::inv(arma::diagmat(sigma2)) * arma::trans(splineS));
  //arma::eig_sym(Lambda2, Q, splineT * arma::inv(arma::diagmat(sigma1)) * arma::trans(splineT));
  //arma::mat precision = arma::kron(P * splineS * Gamma, Q * splineT * Lambda).t() * arma::inv(arma::diagmat((varphi * Identity + arma::diagmat(arma::kron(Lambda1, Lambda2))))) * arma::kron(P * splineS * Gamma, Q * splineT * Lambda) + H;
  //arma::mat Woodbury = arma::eye(splineS.n_rows * splineT.n_rows, splineS.n_rows * splineT.n_rows);
  //precision = arma::kron(arma::trans(Gamma) * arma::trans(splineS), arma::trans(Lambda) * arma::trans(splineT)) * arma::solve(
  //arma::kron(splineS * arma::diagmat(sigma2) * arma::trans(splineS), splineT * arma::diagmat(sigma1) * arma::trans(splineT)) + varphi * Identity,
  //arma::kron(splineS * Gamma, splineT * Lambda));
  //)
  //precision = arma::trans(temp) * temp + H;
  //arma::mat cholwood = arma::chol(sigmat + varphi * arma::kron(arma::trans(splineS) * splineS, arma::trans(splineT) * splineT), "lower");
  precision = arma::trans(BkronGL) *  Woodbury * BkronGL + H;


  cov = arma::inv_sympd(precision);
  arma::mat cholcov = arma::chol(cov, "lower");

  //omp_set_num_threads(12);
  //#pragma omp parallel
  //{
  arma::vec mu;
  //#pragma omp for schedule(static)
  for(arma::uword i = 0; i < n; i++){
    mu = cov * (H * arma::trans(beta)*arma::trans(X.row(i)) + arma::trans(BkronGL) * Woodbury * y.col(i));
    /*mu = cov * (H * arma::trans(beta) * arma::trans(X.row(i)) + arma::kron(P * splineS * Gamma, Q *
     splineT * Lambda).t() * arma::inv(arma::diagmat((varphi * Identity +
     arma::diagmat(arma::kron(Lambda1, Lambda2))))) * arma::kron(P, Q) * y(i));*/
    eta.slice(i) = arma::reshape(mu + cholcov * arma::randn<arma::vec>(arma::uword(q * qstar)), q, qstar);
  }
  //}
  //return eta;
}

void updateEtaSparse(arma::mat &Lambda, arma::mat &Gamma, arma::vec sigma1,
               arma::vec sigma2, arma::mat &H, arma::field<arma::mat> &splineS,
               arma::mat &splineT, arma::field<arma::vec> &y, double varphi,
               arma::mat &beta, arma::mat &X, arma::cube &eta){
  int p = arma::size(sigma1, 0);
  int pstar = arma::size(sigma2, 0);
  int q = arma::size(Lambda, 1);
  int qstar = arma::size(Gamma, 1);
  arma::vec sigvec(p * pstar);
  arma::uword n = arma::size(y, 0);
  arma::mat precision;
  arma::mat cov;
  arma::mat sigmat = arma::diagmat(arma::kron(sigma2, sigma1));


  arma::mat BkronGL;

  arma::mat Identity;
  arma::mat Woodbury;



  arma::mat cholcov;

  //omp_set_num_threads(12);
  //#pragma omp parallel
  //{
  arma::vec mu;
  //#pragma omp for schedule(static)
  for(arma::uword i = 0; i < n; i++){
    BkronGL = arma::kron(splineS(i) * Gamma, splineT * Lambda);
    Identity = arma::eye(splineS(i).n_rows * splineT.n_rows, splineS(i).n_rows * splineT.n_rows);
    Woodbury = varphi * Identity - ::pow(varphi, 2.0) * arma::kron(splineS(i), splineT) *
      arma::inv_sympd(sigmat + varphi * arma::kron(arma::trans(splineS(i)) * splineS(i),
                                                   arma::trans(splineT) * splineT)) * arma::trans(arma::kron(splineS(i), splineT));
    precision = arma::trans(BkronGL) * Woodbury * BkronGL + H;
    cov = arma::inv_sympd(precision);
    cholcov = arma::chol(cov, "lower");
    mu = cov * (H * arma::trans(beta)*arma::trans(X.row(i)) + arma::trans(BkronGL) * Woodbury * y(i));



    /*mu = cov * (H * arma::trans(beta) * arma::trans(X.row(i)) + arma::kron(P * splineS * Gamma, Q *
     splineT * Lambda).t() * arma::inv(arma::diagmat((varphi * Identity +
     arma::diagmat(arma::kron(Lambda1, Lambda2))))) * arma::kron(P, Q) * y(i));*/
    eta.slice(i) = arma::reshape(mu + cholcov * arma::randn<arma::vec>(arma::uword(q * qstar)), q, qstar);
  }
  //}
  //return eta;
  }



// [[Rcpp::plugins(openmp)]]
// [[Rcpp::export]]
void updateGamma(arma::cube &eta, arma::mat &Lambda, arma::vec Deltastar,
                      arma::mat &Phistar, arma::vec sigma1, arma::vec sigma2,
                      arma::cube &theta, arma::mat &Gamma){

  arma::mat EpsLambda = arma::diagmat(arma::sqrt(sigma1)) * Lambda;
  arma::mat bigX(eta.n_slices * Lambda.n_rows, eta.n_cols);
  arma::rowvec cumDelta = arma::conv_to<arma::rowvec>::from(arma::cumprod(Deltastar));
  //arma::mat Gamma(theta.n_cols, eta.n_cols);
  arma::mat muMat = arma::zeros<arma::mat>(Phistar.n_rows, Phistar.n_cols);
  //arma::mat cov(eta.n_cols, eta.n_cols);
  for(arma::uword i = 0; i < eta.n_slices; ++i){

    // Generates concatenated x tilde matrix
    bigX.rows(i * Lambda.n_rows, i * Lambda.n_rows + Lambda.n_rows - 1) = EpsLambda * eta.slice(i);
  }

  bigX = bigX.t() * bigX;
  arma::mat precision(eta.n_cols, eta.n_cols);
  //arma::mat cov;
  //omp_set_num_threads(4);
  //#pragma omp parallel
  //{
    arma::mat cov;
    arma::uword subject;
    //#pragma omp for schedule(static)
    for(arma::uword i = 0; i < Phistar.n_rows; ++i){
      precision = sigma2(i) * bigX + arma::diagmat(cumDelta % Phistar.row(i));
      for( subject = 0; subject < eta.n_slices; ++subject){
        muMat.row(i) = muMat.row(i) + theta.slice(subject).col(i).t() * arma::diagmat(sigma1) * Lambda * eta.slice(subject);
      }
      cov = arma::inv_sympd(precision);
      muMat.row(i) = sigma2(i) * muMat.row(i) * cov;
      Gamma.row(i) = muMat.row(i) + arma::randn<arma::rowvec>(Phistar.n_cols) * arma::chol(cov);
    }
  //}
}

// [[Rcpp::export]]
arma::mat updateH(arma::cube &eta, arma::mat &beta, arma::mat &X){
  // Gives the invserse of H
  double n = eta.n_slices;
  double a = 1;
  double b = 1;
  int q = eta.n_rows;
  int qstar = eta.n_cols;
  arma::mat H(q, qstar);
  //arma::mat sumeta = arma::sum(arma::square(eta), 2);
  //arma::vec veceta = arma::vectorise(sumeta);
  arma::vec z;
  // Do not parallelize with pragma omp parallel for collapse(2)
  for(arma::uword i = 0; i < arma::uword(q); i++){
    for(arma::uword j = 0; j < arma::uword(qstar); j++){
      z = arma::vec(eta.tube(i,j)) - X * beta.col(arma::sub2ind(arma::size(eta.slice(0)), i, j));
      H(i, j) = R::rgamma(a + n/2.0, 1.0 / (b + 1.0/2.0 * arma::sum(z % z)));
    }
  }
  //H = H * H.n_elem / arma::accu(H);
  return arma::diagmat(arma::vectorise(H));

}

// [[Rcpp::export]]
double updateHb(arma::mat H){
  arma::vec Hvec = H.diag();
  // Same a used in updating H
  double a = 1;
  // Priors for rate parameter in H
  double shape = 1;
  double rate = 1;
  double n = Hvec.n_elem;
  return(R::rgamma(n *a + shape, 1.0 / (rate + arma::sum(Hvec))));

}
// [[Rcpp::export]]
arma::vec updateHProd(arma::cube Eta, arma::mat beta, arma::mat X, arma::vec Delta){
  double a = 1;
  double b = 1;
  arma::uvec v(Eta.n_cols - 1);
  for(arma::uword i = 1; i < Eta.n_cols; i++){
    v(i - 1) = i + Eta.n_rows;
  }
  arma::field<arma::uvec> index(Eta.n_rows,Eta.n_cols);
  for(arma::uword i = 0; i < index.n_cols; i++){
    for(arma::uword j = 0; j < index.n_rows; j++){
      index(j, i) = arma::join_cols(arma::regspace<arma::uvec>(1, j + 1), v.head(i)) - 1;
    }
  }
  double sum = 0;
  double n = 0;
  arma::vec ph;
  arma::vec z;
  for(arma::uword i = 0; i < index.n_cols; i++){
    for(arma::uword j = 0; j < index.n_rows; j++){
      ph = Delta.elem(index(j, i));
      z =  (arma::vec(Eta.tube(j, i)) - X * beta.col(arma::sub2ind(arma::size(Eta.slice(0)), j, i)));
      sum = sum + arma::prod(ph.tail(i + j)) * arma::sum(z % z);
      n = n + 1;
    }
  }

  Delta(0) = R::rgamma(a + n * Eta.n_slices / 2.0, 1.0 / (b + 1.0 / 2.0 * sum));
  n = 0;
  sum = 0;
  for(arma::uword k = 1; k < index.n_rows; k++){

    for(arma::uword i = 0; i < index.n_cols; i++){
      for(arma::uword j = k; j < index.n_rows; j++){
        ph = Delta.elem(index(j, i));
        z = (arma::vec(Eta.tube(j, i)) - X * beta.col(arma::sub2ind(arma::size(Eta.slice(0)), j, i)));
        sum = sum + arma::prod(ph.head(k)) * arma::prod(ph.tail(i + j - k)) * arma::sum(z % z);

        n = n + 1;
      }
    }
    //Delta(k) = R::rgamma(a + n * Eta.n_slices / 2.0, 1.0 / (b + 1.0 / 2.0 * sum));
    Delta(k) = sampleTrunc(a + n * Eta.n_slices / 2.0, 1.0 / (b + 1.0 / 2.0 * sum), 1);
    n = 0;
    sum = 0;
  }
  /*
   * something wrong with delta multiplier
   */
  for(arma::uword k = 1; k < index.n_cols; k++){
    for(arma::uword i = k; i < index.n_cols; i++){
      for(arma::uword j = 0; j < index.n_rows; j++){
        ph = Delta.elem(index(j, i));
        //Rcpp::Rcout << "k = " << k << std::endl << arma::prod(ph.head(j + k)) * arma::prod(ph.tail(i - k)) << std::endl;
        z = (arma::vec(Eta.tube(j, i)) - X * beta.col(arma::sub2ind(arma::size(Eta.slice(0)), j, i)));
        sum = sum + arma::prod(ph.head(k + j)) * arma::prod(ph.tail(i - k)) * arma::sum(z % z);
        n = n + 1;
      }
    }

    //Delta(index.n_rows + k - 1) = R::rgamma(a + n * Eta.n_slices / 2.0, 1.0 / (b + 1.0 / 2.0 * sum));
    Delta(index.n_rows + k - 1) = sampleTrunc(a + n * Eta.n_slices / 2.0, 1.0 / (b + 1.0 / 2.0 * sum), 1);

    n = 0;
    sum = 0;
  }
  return(Delta);
}
arma::vec updateHProdOne(arma::mat Eta, arma::vec Delta){
  double a1 = 2;
  double k = Eta.n_cols;
  arma::umat A(k - 1, k - 2);
  for(int col = 0; col < k - 2; ++col){
    for(int row = 0; row < k - 1; ++row){
      if(row > col){
        A(row, col) = col + 1;
      }
      else {
        A(row, col) = col + 2;
      }
    }
  }
  A.insert_cols(0, arma::zeros<arma::uvec>(k-1));
  arma::vec temptau = arma::cumprod(Delta);
  temptau(0) = 1;
  double sum = 0;
  for(arma::uword i = 0; i < Eta.n_cols; i++){
    sum = sum + temptau(i) * arma::norm(Eta.col(i) * arma::norm(Eta.col(i)));
  }
  double rate = 1 + 1.0 / 2.0 * sum;
  double shape = Eta.n_elem / 2.0 + a1;
  Delta(0) = R::rgamma(shape, 1.0 / rate);
  for(arma::uword j = 1; j < Delta.n_elem; j++){
    sum = 0;
    shape = Eta.n_rows * (Eta.n_cols - j + 1) / 2.0 + a1;
    temptau = arma::cumprod(Delta(A.row(j - 1)));
    for(arma::uword h = j; h < Delta.n_elem; h++){
      sum = sum + temptau(h - 1) * arma::dot(Eta.col(h), Eta.col(h));
    }
    rate = 1 + 1.0 / 2.0 * sum;
    Delta(j) = R::rgamma(shape, 1.0 / rate);
  }
  return(Delta);
}
// [[Rcpp::plugins(openmp)]]
// [[Rcpp::export]]
void updateLambda(arma::cube &eta, arma::mat &Gamma, arma::vec Delta,
                       arma::mat &Phi, arma::vec sigma1, arma::vec sigma2, arma::cube &theta, arma::mat &Lambda){

  arma::mat GammaEps = Gamma.t() * arma::diagmat(arma::sqrt(sigma2));
  arma::mat bigX(eta.n_rows, eta.n_slices * Gamma.n_rows);
  arma::rowvec cumDelta = arma::conv_to<arma::rowvec>::from(arma::cumprod(Delta));
  //arma::mat Lambda(theta.n_rows, eta.n_rows);
  arma::mat precision(eta.n_cols, eta.n_cols);
  arma::mat muMat = arma::zeros<arma::mat>(Phi.n_rows, Phi.n_cols);
  arma::mat cov;
  for(arma::uword i = 0; i < eta.n_slices; ++i){

    // Generates concatenated x tilde matrix
    bigX.cols(i * Gamma.n_rows, i * Gamma.n_rows + Gamma.n_rows - 1) = eta.slice(i) * GammaEps;
  }
  bigX = bigX * bigX.t();
  //Geps = arma::diagmat(1.0 / sigma2) * G.t();
  // Covariance matrix for each row of Lambda

  // Mean matrix for Lambda
  for(arma::uword i = 0; i < Phi.n_rows; ++i){

    precision =  sigma1(i) * bigX + arma::diagmat(cumDelta % Phi.row(i));

    for(arma::uword subject = 0; subject < eta.n_slices; ++subject){
      muMat.row(i) = muMat.row(i) + theta.slice(subject).row(i) * arma::diagmat(sigma2) * Gamma * eta.slice(subject).t();
    }
    cov = arma::inv_sympd(precision);
    muMat.row(i) = sigma1(i) * muMat.row(i) * cov;
    Lambda.row(i) = muMat.row(i) + arma::randn<arma::rowvec>(Phi.n_cols) * arma::chol(cov);
  }
}

// [[Rcpp::depends(RcppArmadillo)]]
// [[Rcpp::export]]
arma::mat updateLambda2(arma::mat Theta, arma::mat eta, arma::vec Sigma, arma::vec Tau){
  arma::mat SigmaM = arma::diagmat(Sigma);
  arma::mat Penalty = getPenalty(SigmaM.n_rows);
  arma::mat cov = arma::inv_sympd(arma::kron(arma::trans(eta) * eta, arma::diagmat(Sigma)) + arma::kron(arma::diagmat(Tau), Penalty));
  //arma::mat temp1 = arma::kron(arma::trans(eta) * eta, arma::diagmat(Sigma));
  //arma::mat temp2 = Phi * arma::kron(arma::diagmat(arma::cumprod(Delta)), Penalty);
  //Rcpp::Rcout << "prior precision" << temp2.diag() << std::endl << "data precision" << temp1.diag() << std::endl << "ENNNNNNNNNNDDDDDDDDD";
  //Rcpp::Rcout << "Here";
  arma::vec mu = cov * arma::kron(arma::trans(eta), arma::diagmat(Sigma)) * arma::vectorise(Theta);
  arma::vec Lambda = mu + arma::chol(cov, "lower") * arma::randn<arma::vec>(arma::uword(Sigma.n_elem * Tau.n_elem));
  return(arma::reshape(Lambda, Sigma.n_elem, Tau.n_elem));
}

// [[Rcpp::depends(RcppArmadillo)]]
// [[Rcpp::export]]
arma::mat updateLambda3(arma::mat Theta, arma::mat eta, arma::vec Sigma, arma::mat Phi, arma::vec Delta){

  arma::uword p = Sigma.n_elem;
  arma::rowvec cumDelta = arma::conv_to<arma::rowvec>::from(arma::cumprod(Delta));

  arma::mat SigmaM = arma::diagmat(Sigma);
  arma::mat precision;
  arma::mat cov;
  arma::vec mu;

  arma::mat Lambda(p, Delta.n_elem);
  for(arma::uword i = 0; i < p; i++){

    precision = arma::diagmat(cumDelta % Phi.row(i)) + Sigma(i) * arma::trans(eta) * eta;

    cov = arma::inv_sympd(precision);
    mu = cov * arma::trans(eta) * Sigma(i) * arma::trans(Theta.row(i));
    Lambda.row(i) = arma::trans(mu + arma::chol(cov, "lower") * arma::randn<arma::vec>(Delta.n_elem));
  }

  return(Lambda);
  /*
  arma::mat cov = arma::inv_sympd(arma::kron(arma::trans(eta) * eta, arma::diagmat(Sigma)) + Phi * arma::kron(arma::diagmat(arma::cumprod(Delta)), Penalty));
  arma::mat temp1 = arma::kron(arma::trans(eta) * eta, arma::diagmat(Sigma));
  arma::mat temp2 = Phi * arma::kron(arma::diagmat(arma::cumprod(Delta)), Penalty);
  //Rcpp::Rcout << "prior precision" << temp2.diag() << std::endl << "data precision" << temp1.diag() << std::endl << "ENNNNNNNNNNDDDDDDDDD";
  //Rcpp::Rcout << "Here";
  arma::vec mu = cov * arma::kron(arma::trans(eta), arma::diagmat(Sigma)) * arma::vectorise(Theta);
  arma::vec Lambda = mu + arma::chol(cov, "lower") * arma::randn<arma::vec>(arma::uword(Sigma.n_elem * Delta.n_elem));
  return(arma::reshape(Lambda, Sigma.n_elem, Delta.n_elem));
   */
}


// [[Rcpp::export]]
arma::mat updatePhi(arma::mat &Lambda, arma::vec delta){
  double v = 1;
  arma::vec tauH = arma::cumprod(delta);
  arma::mat Phi(Lambda.n_rows, Lambda.n_cols);
  //omp_set_num_threads(4);
  //#pragma omp for collapse(2) no gain
  for(arma::uword i = 0; i < Phi.n_cols; ++i){
    for(arma::uword j = 0; j < Phi.n_rows; ++j){
      Phi(j,i) = R::rgamma((v + 1.0) / 2.0, 1.0 / (v / 2.0 + 1.0 / 2.0 * tauH(i) * ::pow(Lambda(j,i), 2.0)));
    }
  }
  return Phi;
}


arma::mat updatePhistar(arma::mat &Gamma, arma::vec deltastar){
  double v = 1;
  arma::vec tauH = arma::cumprod(deltastar);
  arma::mat Phistar(Gamma.n_rows, Gamma.n_cols);
  for(arma::uword i = 0; i < Phistar.n_cols; ++i){
    for(arma::uword j = 0; j < Phistar.n_rows; ++j){
      Phistar(j,i) = R::rgamma((v + 1.0) / 2.0, 1.0 / (v / 2.0 + 1.0/2.0 * tauH(i) * ::pow(Gamma(j,i), 2.0)));
    }
  }
  return Phistar;
}

// [[Rcpp::export]]
arma::vec updateSigma1(arma::cube &eta, arma::cube &theta, arma::mat &Lambda, arma::mat &Gamma, arma::vec sigma2){
  double asig1 = .01;
  double bsig1 = .01;
  int p = Lambda.n_rows;
  int pstar = Gamma.n_rows;
  int n = eta.n_slices;
  arma::vec sigma1(p);
  //double ztz = 0;
  //arma::rowvec z;
  arma::mat rooti = arma::sqrt(sigma2);
  //omp_set_num_threads(12);
  //#pragma omp parallel
  //{
    double ztz;
    arma::rowvec z;
    //#pragma omp for schedule(dynamic)
    for(arma::uword j = 0; j < arma::uword(p); j++){
      for(arma::uword i = 0; i < arma::uword(n); i++){
        z = (theta.slice(i).row(j) - ((Lambda.row(j) * eta.slice(i)) * arma::trans(Gamma))) * arma::diagmat(rooti);
        ztz = ztz + arma::sum(z % z);
      }
      sigma1(j) = R::rgamma(asig1 + double(n*pstar)/2.0,  1.0 / (bsig1 + 1.0/2.0 * ztz));
      ztz = 0;
    }
  //}
  return sigma1;
}

arma::vec updateSigma2(arma::cube &eta, arma::cube &theta, arma::mat &Lambda, arma::mat &Gamma, arma::vec sigma1){
  double asig2 = .01;
  double bsig2 = .01;
  int pstar = Gamma.n_rows;
  int n = eta.n_slices;
  int p = Lambda.n_rows;
  arma::vec sigma2(pstar);
  //double ztz = 0;
  //arma::vec z;
  arma::mat rooti = arma::sqrt(sigma1);
  //omp_set_num_threads(12);
  //#pragma omp parallel
  //{
    arma::vec z;
    double ztz;
    //#pragma omp for schedule(dynamic)
    for(arma::uword j = 0; j < arma::uword(pstar); j++){
      for(arma::uword i = 0; i < arma::uword(n); i++){
        z = arma::diagmat(rooti) * (theta.slice(i).col(j) - Lambda * (eta.slice(i) * arma::trans(Gamma.row(j))));
        ztz = ztz + arma::sum(z % z);
      }
      sigma2(j) = R::rgamma(asig2 + double(n*p)/2.0, 1.0 / (bsig2 + 1.0/2.0 * ztz));
      ztz = 0;
    }
  //}
  return sigma2;
}

// [[Rcpp::depends(RcppArmadillo)]]
// [[Rcpp::export]]
arma::vec updateSigma2(arma::mat Theta, arma::mat Lambda, arma::mat Eta){
  arma::vec Sigma(Lambda.n_rows);
  double a = 1;
  double b = 1;
  double shape = a + Theta.n_cols / 2.0;
  double rate = 0;
  double z;
  for(arma::uword j = 0; j < Lambda.n_rows; j++){
    for(arma::uword i = 0; i < Theta.n_cols; i++){
      z = Theta(j, i) - arma::dot(Lambda.row(j), Eta.row(i));
      rate = rate + z * z;
    }
    rate = b + rate / 2.0;
    Sigma(j) = R::rgamma(shape, 1.0 / rate);
    //Rcpp::Rcout << rate << std::endl;
    rate = 0;
  }
  return(Sigma);
}


// [[Rcpp::export]]
arma::cube updateTheta(arma::mat &Lambda, arma::mat &Gamma, arma::vec sigma1,
                       arma::vec sigma2, arma::cube &eta, arma::mat &splineS,
                       arma::mat &splineT, arma::mat &y, double varphi){
  int p = arma::size(sigma1, 0);
  int pstar = arma::size(sigma2, 0);
  //arma::mat spline = arma::kron(splineS, splineT);
  arma::vec sigvec(p * pstar);
  arma::uword n = arma::size(y, 1);
  arma::mat precision;
  //arma::vec mu;
  arma::mat cov;
  arma::cube theta(p, pstar, n);
  arma::mat sigmat = arma::diagmat(arma::kron(sigma2, sigma1));
  arma::mat kronGL = arma::kron(Gamma, Lambda);
  precision = varphi * arma::kron(arma::trans(splineS) * splineS, arma::trans(splineT) * splineT) + sigmat;
  cov = arma::inv_sympd(precision);
  //omp_set_num_threads(12);
  //#pragma omp parallel
  //{
    arma::vec mu;
    //#pragma omp for schedule(dynamic)
    for(arma::uword i = 0; i < n; i++){
      //precision = varphi * arma::trans(spline) * spline + arma::diagmat(sigmat);
      //cov = arma::inv_sympd(precision);
      arma::vec mu;
      mu = cov * (varphi * arma::trans(arma::kron(splineS, splineT)) * y.col(i) + sigmat * kronGL * arma::vectorise(eta.slice(i)));
      theta.slice(i) = arma::reshape(mu + arma::chol(cov, "lower") * arma::randn<arma::vec>(arma::uword(p * pstar)), p, pstar);
    }
  //}
  return theta;
}

// [[Rcpp::depends(RcppArmadillo)]]
// [[Rcpp::export]]
arma::mat updateTheta2(arma::mat Lambda, arma::vec Sigma, arma::mat eta, arma::mat Data, double Varphi, arma::mat Basis){
  arma::mat cov = arma::inv_sympd(Varphi * arma::trans(Basis) * Basis + arma::diagmat(Sigma));
  arma::vec mu;
  arma::mat Theta(Sigma.n_elem, Data.n_cols);
  for(arma::uword i = 0; i < Data.n_cols; i++){
    mu = cov * (Varphi * arma::trans(Basis) * Data.col(i) + arma::diagmat(Sigma) * Lambda * arma::trans(eta.row(i)));
    Theta.col(i) = mu + arma::chol(cov, "lower") * arma::randn<arma::vec>(arma::uword(Sigma.n_elem));
  }
  return(Theta);
}
arma::cube updateThetaSparse(arma::mat &Lambda, arma::mat &Gamma, arma::vec sigma1,
                       arma::vec sigma2, arma::cube &eta, arma::field<arma::mat> &splineS,
                       arma::mat &splineT, arma::field<arma::vec> &y, double varphi){
  int p = arma::size(sigma1, 0);
  int pstar = arma::size(sigma2, 0);
  //arma::mat spline = arma::kron(splineS, splineT);
  arma::vec sigvec(p * pstar);
  arma::uword n = arma::size(y, 0);
  arma::mat precision;
  //arma::vec mu;
  arma::mat cov;
  arma::cube theta(p, pstar, n);
  arma::mat sigmat = arma::diagmat(arma::kron(sigma2, sigma1));
  arma::mat kronGL = arma::kron(Gamma, Lambda);
  //omp_set_num_threads(12);
  //#pragma omp parallel
  //{
  arma::vec mu;
  //#pragma omp for schedule(dynamic)
  for(arma::uword i = 0; i < n; i++){
    //precision = varphi * arma::trans(spline) * spline + arma::diagmat(sigmat);
    //cov = arma::inv_sympd(precision);
    precision = varphi * arma::kron(arma::trans(splineS(i)) * splineS(i), arma::trans(splineT) * splineT) + sigmat;
    cov = arma::inv_sympd(precision);
    mu = cov * (varphi * arma::trans(arma::kron(splineS(i), splineT)) * y(i) + sigmat * kronGL * arma::vectorise(eta.slice(i)));
    theta.slice(i) = arma::reshape(mu + arma::chol(cov, "lower") * arma::randn<arma::vec>(arma::uword(p * pstar)), p, pstar);
  }
  //}
  return theta;
}

// [[Rcpp::export]]
double updateVarphi(arma::cube &theta, arma::mat &splineS, arma::mat &splineT, arma::mat &y){
  double aVarphi = .0001;
  double bVarphi = .0001;
  arma::uword n = theta.n_slices;
  double ztz = 0;
  //arma::vec z;
  double totalN = 0;
  //omp_set_num_threads(12);
  //#pragma omp parallel
  //{
    arma::vec z;
    //#pragma omp for schedule(static) reduction(+:totalN, ztz)
    for(arma::uword i = 0; i < arma::uword(n); i++){
      arma::vec z;
      z = y.col(i) - arma::kron(splineS, splineT) * arma::vectorise(theta.slice(i));
      ztz = ztz + arma::sum(z % z);
      totalN = totalN + splineS.n_rows * splineT.n_rows;
    }
  //}
  return R::rgamma(aVarphi + totalN/2, 1.0 / (bVarphi + 1.0/2.0 * ztz));
}
// [[Rcpp::depends(RcppArmadillo)]]
// [[Rcpp::export]]
double updateVarphi2(arma::mat Data, arma::mat Theta, arma::mat Basis){
  double a = 1;
  double b = 1;
  double shape = a + Data.n_cols * Data.n_rows / 2.0;
  double rate = 0;
  arma::vec z;
  for(arma::uword i = 0; i < Data.n_cols; i++){
    z = Data.col(i) - Basis * Theta.col(i);
    rate = rate + arma::sum(z % z);
  }
  rate = b + rate / 2.0;
  return(R::rgamma(shape, 1.0 / rate));
}


double updateVarphiSparse(arma::cube &theta, arma::field<arma::mat> &splineS, arma::mat &splineT, arma::field<arma::vec> &y){
  double aVarphi = 1;
  double bVarphi = 1;
  arma::uword n = theta.n_slices;
  double ztz = 0;
  //arma::vec z;
  double totalN = 0;
  //omp_set_num_threads(12);
  //#pragma omp parallel
  //{
  arma::vec z;
  //#pragma omp for schedule(static) reduction(+:totalN, ztz)
  for(arma::uword i = 0; i < arma::uword(n); i++){
    arma::vec z;
    z = y(i) - arma::kron(splineS(i), splineT) * arma::vectorise(theta.slice(i));
    ztz = ztz + arma::sum(z % z);
    totalN = totalN + y(i).n_elem;
  }
  //}
  return R::rgamma(aVarphi + totalN/2, 1.0 / (bVarphi + 1.0/2.0 * ztz));
}

arma::vec initializeY(arma::vec x, arma::vec missing, int s, int t){
  arma::vec y(s*t, arma::fill::zeros);
  int counter = 0;
  for(int i = 0; i < s; i++){
    if(arma::any(missing == i+1)){
      y.subvec(t * i, t * i + t - 1).zeros();
    } else {
      y.subvec(t * i, t * i + t - 1) = x.subvec(t * counter,  t * counter + t - 1);
      counter = counter + 1;
    }
  }
  return(y);
}

void updateMissing(arma::mat &y, arma::field<arma::vec> &missing, arma::cube &theta, arma::mat &splineS, arma::mat &splineT, int n){
  for(int i = 0; i < n; i++){
    arma::vec predictedY = arma::kron(splineS, splineT) * arma::vectorise(theta.slice(i));
    arma::vec currentMissing = missing(i);
    for(arma::uword m = 0; m < currentMissing.n_elem; m++){
      y.col(i).subvec(37 * (currentMissing(m)-1), 37 * (currentMissing(m)-1) + 36) =
        predictedY.subvec(37 * (currentMissing(m)-1), 37 * (currentMissing(m)-1) + 36);
    }
  }
}

arma::mat getPenalty(arma::uword n){
  arma::mat Penalty = arma::zeros<arma::mat>(n, n);
  for(arma::uword i = 0; i < n - 1; i++){
    Penalty(i + 1, i) = -1.0;
  }
  for(arma::uword i = 0; i < n - 1; i++){
    Penalty(i, i + 1) = -1.0;
  }
  for(arma::uword i = 1; i < n - 1; i++){
    Penalty(i, i) = 2.0;
  }
  Penalty(0, 0) = 1.0;
  Penalty(n - 1, n - 1) = 1.0;

  return(Penalty);
}

// [[Rcpp::export]]
double updatea1(arma::vec Delta, double a1){
  double a = 1;
  double b = 1;
  double proposal;
  while(true){
    proposal = a1 + R::rnorm(0,1);
    if(proposal > 0){
      break;
    }
  }
  double A = log(R::dgamma(Delta(0), proposal, 1/b, 0)) + log(R::dgamma(proposal, a, 1/b, 0)) +
    log(R::pnorm(a1, 0.0, 1.0, 1, 0)) - log(R::dgamma(Delta(0), a1, 1/b, 0)) - log(R::dgamma(a1, a, 1/b, 0)) -
    log(R::pnorm(proposal, 0.0, 1.0, 1, 0));
  if(R::runif(0.0, 1.0) < exp(A)){
    a1 = proposal;
  }
  return(a1);
}

// [[Rcpp::export]]
double updatea2(arma::vec Delta, double a2){
  double a = 2.0;
  double b = 1.0;
  double proposal;
  int q = arma::size(Delta, 0);
  while(true){
    proposal = a2 + R::rnorm(0,1);
    if(proposal > 0){
      break;
    }
  }
  Rcpp::NumericVector D = Rcpp::wrap(Delta.tail(q - 1));
  double A = Rcpp::sum(Rcpp::log(Rcpp::dgamma(D, proposal, b, false))) + log(R::dgamma(proposal, a, 1/b, 0)) +
    log(R::pnorm(a2, 0.0, 1.0, 1, 0)) - Rcpp::sum(Rcpp::log(Rcpp::dgamma(D, a2, b, false))) - log(R::dgamma(a2, a, 1/b, 0)) - log(R::pnorm(proposal, 0.0, 1.0, 1, 0));
  if(R::runif(0.0, 1.0) < exp(A)){
    a2 = proposal;
  }
  return(a2);
}

// [[Rcpp::export]]
arma::mat updateLambdaSmooth(arma::cube Eta, arma::mat Gamma, arma::vec Sigma1, arma::vec Sigma2, arma::vec Tau, arma::cube Theta){
  arma::mat tempSum(Eta.n_rows, Eta.n_rows);
  arma::mat D = getPenalty(Theta.n_rows);
  tempSum.zeros();

  for(arma::uword i = 0; i < Eta.n_slices; i++){
    tempSum = tempSum + Eta.slice(i) * arma::trans(Gamma) * arma::diagmat(Sigma2) * Gamma * arma::trans(Eta.slice(i));
  }
  arma::mat Precision = arma::kron(tempSum, arma::diagmat(Sigma1)) + arma::kron(arma::diagmat(Tau), D);
  arma::mat Cov = arma::inv_sympd(Precision);


  arma::vec mu(Theta.n_rows * Eta.n_rows);
  mu.zeros();

  for(arma::uword i = 0; i < Eta.n_slices; i++){
    mu = mu + arma::kron(Eta.slice(i) * arma::trans(Gamma) * arma::diagmat(Sigma2), arma::diagmat(Sigma1)) * arma::vectorise(Theta.slice(i));
  }

  return(arma::reshape(Cov * mu + arma::chol(Cov, "lower") * arma::randn<arma::vec>(arma::uword(Theta.n_rows * Eta.n_rows)), Theta.n_rows, Eta.n_rows));

}

// [[Rcpp::export]]
arma::mat updateLambda4(arma::mat Theta, arma::mat eta, arma::vec Sigma, arma::vec Delta){
  arma::mat SigmaM = arma::diagmat(Sigma);
  arma::mat Penalty = getPenalty(SigmaM.n_rows);
  arma::vec Tau = arma::cumprod(Delta);
  arma::mat cov = arma::inv_sympd(arma::kron(arma::trans(eta) * eta, arma::diagmat(Sigma)) + arma::kron(arma::diagmat(Tau), Penalty));
  //arma::mat temp1 = arma::kron(arma::trans(eta) * eta, arma::diagmat(Sigma));
  //arma::mat temp2 = Phi * arma::kron(arma::diagmat(arma::cumprod(Delta)), Penalty);
  //Rcpp::Rcout << "prior precision" << temp2.diag() << std::endl << "data precision" << temp1.diag() << std::endl << "ENNNNNNNNNNDDDDDDDDD";
  //Rcpp::Rcout << "Here";
  arma::vec mu = cov * arma::kron(arma::trans(eta), arma::diagmat(Sigma)) * arma::vectorise(Theta);
  arma::vec Lambda = mu + arma::chol(cov, "lower") * arma::randn<arma::vec>(arma::uword(Sigma.n_elem * Tau.n_elem));
  return(arma::reshape(Lambda, Sigma.n_elem, Tau.n_elem));
}


// [[Rcpp::export]]
arma::mat updateLambdaSmoothD(arma::cube Eta, arma::mat Gamma, arma::vec Sigma1, arma::vec Sigma2, arma::vec Delta, arma::cube Theta){
  arma::mat tempSum(Eta.n_rows, Eta.n_rows);
  arma::mat D = getPenalty(Theta.n_rows);
  tempSum.zeros();
  arma::vec Tau = arma::cumprod(Delta);
  for(arma::uword i = 0; i < Eta.n_slices; i++){
    tempSum = tempSum + Eta.slice(i) * arma::trans(Gamma) * arma::diagmat(Sigma2) * Gamma * arma::trans(Eta.slice(i));
  }
  arma::mat Precision = arma::kron(tempSum, arma::diagmat(Sigma1)) + arma::kron(arma::diagmat(Tau), D);
  arma::mat Cov = arma::inv_sympd(Precision);


  arma::vec mu(Theta.n_rows * Eta.n_rows);
  mu.zeros();

  for(arma::uword i = 0; i < Eta.n_slices; i++){
    mu = mu + arma::kron(Eta.slice(i) * arma::trans(Gamma) * arma::diagmat(Sigma2), arma::diagmat(Sigma1)) * arma::vectorise(Theta.slice(i));
  }

  return(arma::reshape(Cov * mu + arma::chol(Cov, "lower") * arma::randn<arma::vec>(arma::uword(Theta.n_rows * Eta.n_rows)), Theta.n_rows, Eta.n_rows));

}

// [[Rcpp::export]]
arma::mat updateGammaSmooth(arma::cube Eta, arma::mat Lambda, arma::vec Sigma1, arma::vec Sigma2, arma::vec Tau, arma::cube Theta){
  arma::mat tempSum(Eta.n_cols, Eta.n_cols);
  arma::mat D = getPenalty(Theta.n_cols);
  tempSum.zeros();

  for(arma::uword i = 0; i < Eta.n_slices; i++){
    tempSum = tempSum + arma::trans(Lambda * Eta.slice(i)) * arma::diagmat(Sigma1) * Lambda * Eta.slice(i);
  }

  arma::mat Precision = arma::kron(tempSum, arma::diagmat(Sigma2)) + arma::kron(arma::diagmat(Tau), D);

  arma::mat Cov = arma::inv_sympd(Precision);

  arma::vec mu(Theta.n_cols * Eta.n_cols);

  mu.zeros();
  for(arma::uword i = 0; i < Eta.n_slices; i++){
    mu = mu + arma::kron(arma::trans(Lambda * Eta.slice(i)) * arma::diagmat(Sigma1), arma::diagmat(Sigma2)) * arma::vectorise(arma::trans(Theta.slice(i)));
  }

  return(arma::reshape(Cov * mu + arma::chol(Cov, "lower") * arma::randn<arma::vec>(arma::uword(Theta.n_cols * Eta.n_cols)), Theta.n_cols, Eta.n_cols));
}

// [[Rcpp::export]]
arma::mat updateGammaSmoothD(arma::cube Eta, arma::mat Lambda, arma::vec Sigma1, arma::vec Sigma2, arma::vec Delta, arma::cube Theta){
  arma::mat tempSum(Eta.n_cols, Eta.n_cols);
  arma::mat D = getPenalty(Theta.n_cols);
  tempSum.zeros();
  arma::vec Tau = arma::cumprod(Delta);
  for(arma::uword i = 0; i < Eta.n_slices; i++){
    tempSum = tempSum + arma::trans(Lambda * Eta.slice(i)) * arma::diagmat(Sigma1) * Lambda * Eta.slice(i);
  }

  arma::mat Precision = arma::kron(tempSum, arma::diagmat(Sigma2)) + arma::kron(arma::diagmat(Tau), D);

  arma::mat Cov = arma::inv_sympd(Precision);

  arma::vec mu(Theta.n_cols * Eta.n_cols);

  mu.zeros();
  for(arma::uword i = 0; i < Eta.n_slices; i++){
    mu = mu + arma::kron(arma::trans(Lambda * Eta.slice(i)) * arma::diagmat(Sigma1), arma::diagmat(Sigma2)) * arma::vectorise(arma::trans(Theta.slice(i)));
  }

  return(arma::reshape(Cov * mu + arma::chol(Cov, "lower") * arma::randn<arma::vec>(arma::uword(Theta.n_cols * Eta.n_cols)), Theta.n_cols, Eta.n_cols));
}
// [[Rcpp::export]]
arma::vec updateTau(arma::mat Lambda){
  double a = 1;
  double b = 1;
  arma::mat Penalty = getPenalty(Lambda.n_rows);
  arma::vec Tau(Lambda.n_cols);
  double shape = Lambda.n_rows / 2 + a;
  double rate;
  for(arma::uword j = 0; j < Lambda.n_cols; j++){
    rate = b + 1.0 / 2.0 * arma::as_scalar(arma::trans(Lambda.col(j)) * Penalty * Lambda.col(j));
    Tau(j) = R::rgamma(shape, 1.0 / rate);
  }
  return(Tau);
}
// [[Rcpp::export]]
arma::vec updateDeltaProd(arma::mat Lambda, arma::vec Delta){
  double a = 1;
  double b = 1;
  /*
  arma::uword j = 5;
  arma::uvec phvec(Lambda.n_cols - 1);
  phvec.subvec(0, j - 1) = arma::regspace<arma::uvec>(0, j - 1);
  if(j < Lambda.n_cols - 1){
    phvec.subvec(j, Lambda.n_cols - 2) = arma::regspace<arma::uvec>(j + 1, Lambda.n_cols - 1);
  }
  arma::vec temp = arma::cumprod(Tau.elem(phvec));
  Rcpp::Rcout << temp.tail(Lambda.n_cols - j);
  temp = 1 << 2;
  Rcpp::Rcout << std::endl << temp;
  */
  arma::mat Penalty = getPenalty(Lambda.n_rows);
  arma::vec TauTemp = Delta;
  TauTemp(0) = 1.0;
  double rate = 0;
  TauTemp = arma::cumprod(TauTemp);
  for(arma::uword j = 0; j < Lambda.n_cols; j++){
    rate = 1.0 / 2.0 * arma::as_scalar(arma::trans(Lambda.col(j)) * Penalty * TauTemp(j) * Lambda.col(j));
  }
  rate = rate + b;
  double shape = Lambda.n_rows * Lambda.n_cols / 2 + a;
  Delta(0) = R::rgamma(shape, 1.0 / rate);
  arma::vec Tau;
  rate = 0;
  arma::uvec phvec(Lambda.n_cols - 1);
  for(arma::uword j = 1; j < Lambda.n_cols; j++){
    shape = a + Lambda.n_rows * (Lambda.n_cols - j) / 2.0;
    phvec.subvec(0, j - 1) = arma::regspace<arma::uvec>(0, j - 1);
    if(j < Lambda.n_cols - 1){
      phvec.subvec(j, Lambda.n_cols - 2) = arma::regspace<arma::uvec>(j + 1, Lambda.n_cols - 1);
    }
    Tau = arma::cumprod(Delta.elem(phvec));
    for(arma::uword k = j + 1; k < Lambda.n_cols; k++){
      rate = rate + 1.0 / 2.0 * arma::as_scalar(arma::trans(Lambda.col(k)) * Penalty * Tau(k - 2) * Lambda.col(k));
    }

    rate = rate + b;
    Delta(j) = R::rgamma(shape, 1.0 / rate);
    rate = 0;
  }
  return(Delta);
}

arma::mat convertToPrecision(arma::vec Delta, arma::uword q1, arma::uword q2){
  arma::mat H(q1, q2);
  arma::uvec v(q2 - 1);
  for(arma::uword i = 1; i < arma::uword(q2); i++){
    v(i - 1) = i + q1;
  }
  arma::field<arma::uvec> index(q1, q2);
  for(arma::uword i = 0; i < index.n_cols; i++){
    for(arma::uword j = 0; j < index.n_rows; j++){
      index(j, i) = arma::join_cols(arma::regspace<arma::uvec>(1, j + 1), v.head(i)) - 1;
    }
  }
  for(arma::uword i = 0; i < index.n_cols; i++){
    for(arma::uword j = 0; j < index.n_rows; j++){
      H(j, i) = arma::prod(Delta.elem(index(j, i)));
    }
  }
  return(arma::diagmat(arma::vectorise(H)));
}

double sampleTrunc(double shape, double scale, double cutoff){
  bool valid = false;
  double proposal;
  while(!valid){
    proposal = R::rgamma(shape, scale);
    if(proposal > cutoff){
      valid = true;
    }
  }
  return(proposal);
}
